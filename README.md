jtbeta.zip File Generator

Una utilidad escrita en C para calcular la secuencia de 4 bytes correspondiente a un hash CRC-32 específico y empaquetarlos automáticamente en el archivo jtbeta.zip.

Instalación y Compilación:

Solo necesitas un compilador de C en Windows (GCC recomendado).

PowerShell/CMD con MinGW:

```gcc jtbetagen.c resource.res -o jtbetagen.exe -lole32 -luuid -lcomdlg32 -mwindows```
<br/>
<br/>
Uso:
<br/>
<br/>
Al ejecutar el programa solicitará abrir el archivo .mra beta:
<br/>
<br/>
<img src="https://github.com/user-attachments/assets/c99f5225-c201-434f-a984-982a3c7bbfc9" />
<br/>
<br/>
<br/>
<br/>
Después de abrir el archivo .mra beta solicitará guardar el archivo jtbeta.zip:
<br/>
<br/>
<img src="https://github.com/user-attachments/assets/e7149e32-e9d0-4b1a-bd43-65f2e698082a" />
<br/>
<br/>
<br/>
<br/>
El programa calculará los bytes necesarios y se creará el archivo jtbeta.zip:
<br/>
<br/>
<img src="https://github.com/user-attachments/assets/99d29b45-ced4-4269-8bfc-8af01e6311d4" />
<br/>
<br/>
<br/>
<br/>
Si el archivo .mra no es beta o no es válido aparecerá un mensaje de error y terminará el programa.
<br/>
<br/>
<img src="https://github.com/user-attachments/assets/d17bff09-0d42-4181-8961-f4a6674373e4" />
