# frozen_string_literal: true

require 'zlib'

module LiteRGSSTools
  # Just enough PNG for litergss-scene's diffs: every non-interlaced format
  # LiteRGSS' to_png_file writes (8-bit gray / RGB / RGBA, and the palette or
  # low-depth gray lodepng picks for few-colour frames), decoded to an RGBA
  # byte string; and an RGBA encoder for the diff image. No external tool.
  module PNG
    SIGNATURE = "\x89PNG\r\n\x1a\n".b
    Image = Struct.new(:width, :height, :rgba) do
      def pixel(x, y) = rgba.byteslice((y * width + x) * 4, 4).bytes
    end

    module_function

    CHANNELS = { 0 => 1, 2 => 3, 3 => 1, 4 => 2, 6 => 4 }.freeze

    def read(path)
      data = File.binread(path)
      raise Error, "#{path}: not a PNG" unless data.start_with?(SIGNATURE)

      pos = 8
      idat = +''.b
      palette = []
      alphas = []
      width = height = depth = color = interlace = nil
      while pos < data.bytesize
        length = data.byteslice(pos, 4).unpack1('N')
        type = data.byteslice(pos + 4, 4)
        body = data.byteslice(pos + 8, length)
        case type
        when 'IHDR' then width, height, depth, color, _, _, interlace = body.unpack('NNCCCCC')
        when 'PLTE' then palette = body.bytes.each_slice(3).to_a
        when 'tRNS' then alphas = body.bytes
        when 'IDAT' then idat << body
        when 'IEND' then break
        end
        pos += 12 + length
      end
      channels = CHANNELS[color]
      unless channels && interlace.zero? && (depth == 8 || ([0, 3].include?(color) && [1, 2, 4].include?(depth)))
        raise Error, "#{path}: unsupported PNG (depth #{depth}, color type #{color}, interlace #{interlace})"
      end

      rows = unfilter(Zlib::Inflate.inflate(idat), width, height, channels * depth)
      Image.new(width, height, to_rgba(rows, width, color, depth, palette, alphas))
    end

    # Raw scanlines (filters undone), one byte string per row.
    def unfilter(raw, width, height, bits_per_pixel)
      stride = (width * bits_per_pixel + 7) / 8
      bpp = [bits_per_pixel / 8, 1].max
      previous = Array.new(stride, 0)
      (0...height).map do |y|
        filter = raw.getbyte(y * (stride + 1))
        line = raw.byteslice(y * (stride + 1) + 1, stride).bytes
        line.each_index do |i|
          a = i >= bpp ? line[i - bpp] : 0
          b = previous[i]
          c = i >= bpp ? previous[i - bpp] : 0
          line[i] = (line[i] + case filter
                               when 0 then 0
                               when 1 then a
                               when 2 then b
                               when 3 then (a + b) / 2
                               when 4 then paeth(a, b, c)
                               else raise Error, "bad PNG filter #{filter}"
                               end) & 0xFF
        end
        previous = line
      end
    end

    def to_rgba(rows, width, color, depth, palette, alphas)
      out = String.new(capacity: width * rows.size * 4, encoding: Encoding::BINARY)
      rows.each do |line|
        samples = depth == 8 ? line : line.flat_map { |byte| (8 / depth).times.map { |k| (byte >> (8 - depth * (k + 1))) & ((1 << depth) - 1) } }
        pixels = case color
                 when 6 then samples.first(width * 4)
                 when 2 then samples.first(width * 3).each_slice(3).flat_map { |px| px << 255 }
                 when 4 then samples.first(width * 2).each_slice(2).flat_map { |g, al| [g, g, g, al] }
                 when 0
                   scale = 255 / ((1 << depth) - 1)
                   samples.first(width).flat_map { |g| [g * scale] * 3 << 255 }
                 when 3 then samples.first(width).flat_map { |i| (palette[i] || [0, 0, 0]) + [alphas.fetch(i, 255)] }
                 end
        out << pixels.pack('C*')
      end
      out
    end

    def paeth(a, b, c)
      p = a + b - c
      pa = (p - a).abs
      pb = (p - b).abs
      pc = (p - c).abs
      return a if pa <= pb && pa <= pc

      pb <= pc ? b : c
    end

    def write(path, width, height, rgba)
      rows = (0...height).map { |y| "\x00".b + rgba.byteslice(y * width * 4, width * 4) }.join
      chunk = ->(type, body) { [body.bytesize].pack('N') + type + body + [Zlib.crc32(type + body)].pack('N') }
      File.binwrite(path, SIGNATURE + chunk.('IHDR', [width, height, 8, 6, 0, 0, 0].pack('NNCCCCC')) +
                          chunk.('IDAT', Zlib::Deflate.deflate(rows)) + chunk.('IEND', ''.b))
    end
  end
end
