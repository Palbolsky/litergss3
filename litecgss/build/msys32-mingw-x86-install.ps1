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

# install latest pacman
bash_exec 'pacman -Sy --noconfirm pacman pacman-mirrors'

# update core packages
bash_exec 'pacman -Syu --noconfirm'
bash_exec 'pacman -Syu --noconfirm'

# install packages
bash_exec 'pacman --sync --noconfirm mingw-w64-i686-toolchain'
bash_exec 'pacman --sync --noconfirm make'
