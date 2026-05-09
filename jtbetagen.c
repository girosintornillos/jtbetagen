/*
 * Project: CRC32 to ZIP Byte Generator
 * Description: Calculates the 4-byte sequence for a target CRC-32 and 
 *              generates a 'jtbeta.zip' containing a 'beta.bin' file.
 * 
 * Author: GiRo SiNToRNiLLoS™
 * Date: Mayo 2026
 * License: MIT
 * 
 * Notes: 
 * - Compatible with GCC (Linux/Windows).
 * - Uses a manual ZIP structure to avoid external library dependencies.
 * - Inverts the CRC-32 polynomial mapping in O(1).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Tablas para el cálculo directo e inverso del CRC-32
uint32_t table[256];
uint8_t revT[256];

void init_tables() {
    for (int i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320 ^ (c >> 1);
            else c >>= 1;
        }
        table[i] = c;
        // El byte más significativo de c es un mapeo 1:1, nos permite revertir el proceso
        revT[c >> 24] = i; 
    }
}

// Funciones auxiliares para escribir en Little-Endian (formato requerido por ZIP)
void write_le32(FILE *f, uint32_t val) {
    uint8_t buf[4] = { val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF };
    fwrite(buf, 1, 4, f);
}

void write_le16(FILE *f, uint16_t val) {
    uint8_t buf[2] = { val & 0xFF, (val >> 8) & 0xFF };
    fwrite(buf, 1, 2, f);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <CRC32 en hex>\n", argv[0]);
        printf("Ejemplo: %s 032970d5\n", argv[0]);
        return 1;
    }

    // Leer el CRC desde la línea de comandos
    uint32_t target_crc = (uint32_t)strtoul(argv[1], NULL, 16);

    init_tables();

    // ---------------------------------------------------------
    // ALGORITMO INVERSO PARA OBTENER LOS 4 BYTES DESDE EL CRC32
    // ---------------------------------------------------------
    uint32_t S4 = target_crc ^ 0xFFFFFFFF;

    uint8_t b3 = S4 >> 24;
    uint8_t I3 = revT[b3];
    uint32_t R3 = S4 ^ table[I3];

    uint8_t b2 = (R3 >> 16) & 0xFF;
    uint8_t I2 = revT[b2];
    uint32_t R2 = R3 ^ (table[I2] >> 8);

    uint8_t b1 = (R2 >> 8) & 0xFF;
    uint8_t I1 = revT[b1];
    uint32_t R1 = R2 ^ (table[I1] >> 16);

    uint8_t b0 = R1 & 0xFF;
    uint8_t I0 = revT[b0];

    uint32_t S0 = 0xFFFFFFFF;
    uint8_t B0 = (S0 & 0xFF) ^ I0;
    uint32_t S1 = (S0 >> 8) ^ table[I0];
    uint8_t B1 = (S1 & 0xFF) ^ I1;
    uint32_t S2 = (S1 >> 8) ^ table[I1];
    uint8_t B2 = (S2 & 0xFF) ^ I2;
    uint32_t S3 = (S2 >> 8) ^ table[I2];
    uint8_t B3 = (S3 & 0xFF) ^ I3;

    uint8_t data[4] = {B0, B1, B2, B3};
    printf("Bytes calculados: %02X %02X %02X %02X\n", B0, B1, B2, B3);

    // ---------------------------------------------------------
    // CREACIÓN DEL ARCHIVO ZIP (SIN COMPRESIÓN)
    // ---------------------------------------------------------
    FILE *f = fopen("jtbeta.zip", "wb");
    if (!f) {
        printf("Error al crear jtbeta.zip\n");
        return 1;
    }

    // 1. Local File Header (38 bytes)
    write_le32(f, 0x04034b50); // Firma PK\x03\x04
    write_le16(f, 10);         // Versión necesaria (1.0)
    write_le16(f, 0);          // Flags
    write_le16(f, 0);          // Compresión (0 = Store)
    write_le16(f, 0x1400);     // Hora MS-DOS (02:32:00)
    write_le16(f, 0x2199);     // Fecha MS-DOS (25/12/1996)
    write_le32(f, target_crc); // CRC-32 de los datos
    write_le32(f, 4);          // Tamaño comprimido
    write_le32(f, 4);          // Tamaño real
    write_le16(f, 8);          // Longitud del nombre
    write_le16(f, 0);          // Longitud extra
    fwrite("beta.bin", 1, 8, f); // Nombre del archivo (8 bytes)

    // 2. Data (4 bytes)
    fwrite(data, 1, 4, f);     // Los bytes generados

    // 3. Central Directory Header (54 bytes)
    write_le32(f, 0x02014b50); // Firma PK\x01\x02
    write_le16(f, 20);         // Versión del creador (2.0)
    write_le16(f, 10);         // Versión necesaria
    write_le16(f, 0);          // Flags
    write_le16(f, 0);          // Compresión (Store)
    write_le16(f, 0x1400);     // Hora MS-DOS (02:32:00)
    write_le16(f, 0x2199);     // Fecha MS-DOS (25/12/1996)
    write_le32(f, target_crc); // CRC-32
    write_le32(f, 4);          // Tamaño comprimido
    write_le32(f, 4);          // Tamaño real
    write_le16(f, 8);          // Longitud del nombre
    write_le16(f, 0);          // Campo extra
    write_le16(f, 0);          // Comentario
    write_le16(f, 0);          // Número de disco
    write_le16(f, 0);          // Atributos internos
    write_le32(f, 0);          // Atributos externos
    write_le32(f, 0);          // Offset del Local Header (0)
    fwrite("beta.bin", 1, 8, f); // Nombre del archivo

    // 4. End of Central Directory Record (22 bytes)
    write_le32(f, 0x06054b50); // Firma PK\x05\x06
    write_le16(f, 0);          // Disco
    write_le16(f, 0);          // Disco inicio CD
    write_le16(f, 1);          // Registros CD en este disco
    write_le16(f, 1);          // Registros CD totales
    write_le32(f, 54);         // Tamaño del Central Directory (46 fijo + 8 nombre)
    write_le32(f, 42);         // Offset del Central Directory (38 Local Header + 4 Data)
    write_le16(f, 0);          // Tamaño comentario del ZIP

    fclose(f);

    printf("Archivo 'jtbeta.zip' generado con exito (conteniendo beta.bin).\n");
    return 0;
}
