/**
 * Project: jtbeta.zip File Generator
 * Description: Calculates the 4-byte sequence for a target CRC-32 and 
 *              generates a 'jtbeta.zip' containing a 'beta.bin' file.
 *
 * Author: GiRo SiNToRNiLLoS™
 * Date: Mayo 2026
 * License: MIT
 *
 * Notes: 
 * - Compatible with GCC (Linux/Windows) and ARM architectures.
 * - Uses a manual ZIP structure to avoid external library dependencies.
 * - Inverts the CRC-32 polynomial mapping in O(1) using precomputed tables.
 * - Implements TorrentZip standard (UTC 1996-12-24 23:32:00) for deterministic output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// --- Constantes TorrentZip (24/12/1996 23:32:00) ---
#define TRZ_TIME 0xBC00
#define TRZ_DATE 0x2198

uint32_t table[256];
uint8_t revT[256];

/**
 * Inicializa las tablas para cálculo directo e inverso de CRC-32.
 */
void init_tables() {
    for (int i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1) c = 0xEDB88320 ^ (c >> 1);
            else c >>= 1;
        }
        table[i] = c;
        revT[c >> 24] = i; 
    }
}

/**
 * Calcula los 4 bytes necesarios para producir un CRC-32 objetivo.
 */
void solve_crc32(uint32_t target, uint8_t *out_bytes) {
    uint32_t S4 = target ^ 0xFFFFFFFF;

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
    out_bytes[0] = (S0 & 0xFF) ^ I0;
    uint32_t S1 = (S0 >> 8) ^ table[I0];
    out_bytes[1] = (S1 & 0xFF) ^ I1;
    uint32_t S2 = (S1 >> 8) ^ table[I1];
    out_bytes[2] = (S2 & 0xFF) ^ I2;
    uint32_t S3 = (S2 >> 8) ^ table[I2];
    out_bytes[3] = (S3 & 0xFF) ^ I3;
}

/**
 * Función para calcular el CRC-32 del Central Directory.
 */
uint32_t crc32_block(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * Funciones auxiliares para escribir en Little-Endian (formato requerido por ZIP).
 */
void write_le32(uint8_t **p, uint32_t v) {
    (*p)[0] = v & 0xFF; (*p)[1] = (v >> 8) & 0xFF;
    (*p)[2] = (v >> 16) & 0xFF; (*p)[3] = (v >> 24) & 0xFF;
    *p += 4;
}

void write_le16(uint8_t **p, uint16_t v) {
    (*p)[0] = v & 0xFF; (*p)[1] = (v >> 8) & 0xFF;
    *p += 2;
}

int main(int argc, char *argv[]) {
    if (argc != 2 || strlen(argv[1]) != 8) {
        fprintf(stderr, "Uso: %s <CRC32 hex de 8 caracteres>\n", argv[0]);
        return 1;
    }

    // Validación de la entrada hexadecimal
    char *endptr;
    uint32_t target_crc = (uint32_t)strtoul(argv[1], &endptr, 16);
    if (*endptr != '\0') {
        fprintf(stderr, "Error: El valor contiene caracteres no hexadecimales.\n");
        return 1;
    }

    init_tables();
    uint8_t data[4];
    solve_crc32(target_crc, data);

    printf("CRC Target: %08X -> Bytes: %02X %02X %02X %02X\n", target_crc, data[0], data[1], data[2], data[3]);

    FILE *f = fopen("jtbeta.zip", "wb");
    if (!f) {
        perror("Error al crear jtbeta.zip");
        return 1;
    }

    // --- Local File Header ---
    uint8_t lfh[30];
    uint8_t *ptr = lfh;
    write_le32(&ptr, 0x04034b50); 
    write_le16(&ptr, 10);
    write_le16(&ptr, 0);
    write_le16(&ptr, 0);
    write_le16(&ptr, TRZ_TIME);
    write_le16(&ptr, TRZ_DATE);
    write_le32(&ptr, target_crc); 
    write_le32(&ptr, 4);
    write_le32(&ptr, 4);
    write_le16(&ptr, 8);
    write_le16(&ptr, 0);
    
    fwrite(lfh, 1, 30, f);
    fwrite("beta.bin", 1, 8, f);
    fwrite(data, 1, 4, f);

    // --- Central Directory ---
    uint8_t cd[54];
    uint8_t *ptr_cd = cd;
    write_le32(&ptr_cd, 0x02014b50);
    write_le16(&ptr_cd, 20);
    write_le16(&ptr_cd, 10);
    write_le16(&ptr_cd, 0);
    write_le16(&ptr_cd, 0);
    write_le16(&ptr_cd, TRZ_TIME);
    write_le16(&ptr_cd, TRZ_DATE);
    write_le32(&ptr_cd, target_crc);
    write_le32(&ptr_cd, 4);
    write_le32(&ptr_cd, 4);
    write_le16(&ptr_cd, 8);
    write_le16(&ptr_cd, 0);
    write_le16(&ptr_cd, 0);
    write_le16(&ptr_cd, 0);
    write_le16(&ptr_cd, 0);
    write_le32(&ptr_cd, 0);
    write_le32(&ptr_cd, 0);
    memcpy(ptr_cd, "beta.bin", 8);

    uint32_t cd_crc = crc32_block(cd, 54);

    // --- Comentario y EOCD ---
    char comment[23];
    snprintf(comment, sizeof(comment), "TORRENTZIPPED-%08X", cd_crc);

    uint8_t eocd[22];
    uint8_t *ptr_e = eocd;
    write_le32(&ptr_e, 0x06054b50);
    write_le16(&ptr_e, 0);
    write_le16(&ptr_e, 0);
    write_le16(&ptr_e, 1);
    write_le16(&ptr_e, 1);
    write_le32(&ptr_e, 54);
    write_le32(&ptr_e, 42);
    write_le16(&ptr_e, 22);

    fwrite(cd, 1, 54, f);
    fwrite(eocd, 1, 22, f);
    fwrite(comment, 1, 22, f);

    fclose(f);
    printf("Archivo 'jtbeta.zip' generado con exito.\n");
    return 0;
}
