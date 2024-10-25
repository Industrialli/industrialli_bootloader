# Industrialli bootloader

<p align="center">
  <img src="https://github.com/Industrialli/industrialli_bootloader/blob/Arduino/flash_organization.png" />
</p>

O arquivo platformio.ini do programa principal deve possuir as seguintes configurações:
- board_upload.maximum_size = 393216
- board_build.flash_offset = 0x20000
