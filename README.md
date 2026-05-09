jtbeta.zip File Generator

Una utilidad ligera escrita en C para calcular la secuencia de 4 bytes correspondiente a un hash CRC-32 específico y empaquetarlos automáticamente en el archivo jtbeta.zip.

Instalación y Compilación:

Solo necesitas un compilador de C (GCC recomendado).

En Linux

gcc -O2 jtbetagen.c -o jtbetagen

En Windows (PowerShell/CMD con MinGW)

gcc -O2 jtbetagen.c -o jtbetagen.exe

Uso:

Primero se debe averiguar que código CRC-32 utilizar, este CRC-32 se encuentra en los .mra beta:

  <part name="beta.bin" crc="xxxxxxxx"/>

Ejecuta el programa pasando el código CRC-32 en formato hexadecimal como argumento, en este ejemplo se usará el CRC-32 "032970d5":

./jtbetagen 032970d5

Resultado:

  El programa calculará los bytes (en el ejemplo: 3B 28 14 10).
  
  Se creará un archivo llamado jtbeta.zip.
  
  Dentro del ZIP encontrarás beta.bin con el contenido.

Licencia

Este proyecto está bajo la Licencia MIT. Siéntete libre de usarlo, modificarlo y distribuirlo.
