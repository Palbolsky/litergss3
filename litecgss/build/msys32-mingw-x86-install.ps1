$current_folder = (Get-Item .).FullName

function bash_exec($command) {
    Write-Host $command
    & "$current_folder\bash.exe" --login -c "$command"
    Write-Host " - OK" -ForegroundColor Green
}

[Environment]::SetEnvironmentVariable("MSYS2_PATH_TYPE", "inherit", "Machine")

bash_exec 'curl -O http://repo.msys2.org/msys/x86_64/msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz'
bash_exec 'curl -O http://repo.msys2.org/msys/x86_64/msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz.sig'

bash_exec 'pacman-key --verify msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz.sig'
bash_exec 'pacman -U --noconfirm --config <(echo) msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz'
bash_exec 'pacman -U --noconfirm msys2-keyring-r21.b39fb11-1-any.pkg.tar.xz'

bash_exec 'pacman-key --init'
bash_exec 'pacman-key --populate msys2'

# install packages
bash_exec 'pacman -S --needed --noconfirm \
autoconf autoconf2.13 autogen automake-wrapper automake1.10 automake1.11 automake1.12 automake1.13 \
automake1.14 automake1.15 automake1.6 automake1.7 automake1.8 automake1.9 diffutils file gawk grep \
libtool m4 make patch pkg-config sed texinfo texinfo-tex wget mingw-w64-i686-binutils mingw-w64-i686-crt-git \
mingw-w64-i686-gcc mingw-w64-i686-gcc-libs mingw-w64-i686-headers-git mingw-w64-i686-libmangle-git \
mingw-w64-i686-libwinpthread-git mingw-w64-i686-make mingw-w64-i686-pkg-config mingw-w64-i686-tools-git \
mingw-w64-i686-winpthreads-git'

bash_exec "ps x | awk 'NR>1 {print `$1}' | xargs kill -9"
