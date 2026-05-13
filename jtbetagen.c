/*
 * Project: jtbeta.zip File Generator
 * Description: Calculates the 4-byte sequence for a target CRC-32 and 
 *              generates a 'jtbeta.zip' containing a 'beta.bin' file.
 *
 * Author: GiRo SiNToRNiLLoS™
 * Date: Mayo 2026
 * License: MIT
 *
 * Notes: 
 * - Uses a manual ZIP structure to avoid external library dependencies.
 * - Inverts the CRC-32 polynomial mapping in O(1) using precomputed tables.
 * - Implements TorrentZip standard (UTC 1996-12-24 23:32:00) for deterministic output.
 */

#define COBJMACROS
#include <windows.h>
#include <shobjidl.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

// --- Constantes TorrentZip (24/12/1996 23:32:00) y Tablas ---
#define TRZ_TIME 0xBC00
#define TRZ_DATE 0x2198
uint32_t table[256];
uint8_t revT[256];

/**
 * Inicializa las tablas para cálculo directo e inverso de CRC-32
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
 * Calcula los 4 bytes necesarios para producir el CRC-32 objetivo
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
 * Función para calcular el CRC-32 del Central Directory
 */
uint32_t crc32_block(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * Funciones auxiliares para escribir en Little-Endian (formato requerido por ZIP)
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

/**
 * Función para buscar el CRC en el archivo .mra
 */
int get_crc_from_mra(const char* filename, uint32_t* target_crc) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    char line[1024];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char* part_ptr = strstr(line, "<part name=\"beta.bin\"");
        if (part_ptr) {
            char* crc_ptr = strstr(part_ptr, "crc=\"");
            if (crc_ptr) {
                crc_ptr += 5; // Saltar crc="
                
                // --- VALIDACIÓN ---
                for (int i = 0; i < 8; i++) {
                    // Si el caracter no es hexadecimal o llegamos al final de la línea antes de tiempo
                    if (!isxdigit((unsigned char)crc_ptr[i])) {
                        fclose(f);
                        return 0; // Cadena no válida, salimos con error
                    }
                }

                char hex[9] = {0};
                strncpy(hex, crc_ptr, 8);
                
                // Usamos endptr para una validación extra de conversión
                char *endptr;
                *target_crc = (uint32_t)strtoul(hex, &endptr, 16);
                
                if (*endptr == '\0') {
                    found = 1;
                }
                break;
            }
        }
    }
    fclose(f);
    return found;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return 1;

	uint32_t target_crc = 0;
	
    // --- IFileOpenDialog para el .mra ---
    IFileOpenDialog *pfd = NULL;
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileOpenDialog, (void**)&pfd);
    
    if (SUCCEEDED(hr)) {
        pfd->lpVtbl->SetTitle(pfd, L"Seleccionar archivo MRA");
        COMDLG_FILTERSPEC rgSpec[] = {{L"MiSTer Arcade Files", L"*.mra"}};
		pfd->lpVtbl->SetFileTypes(pfd, 1, rgSpec);

        hr = pfd->lpVtbl->Show(pfd, NULL);
        if (SUCCEEDED(hr)) {
            IShellItem *psi = NULL;
            hr = pfd->lpVtbl->GetResult(pfd, &psi);
            if (SUCCEEDED(hr)) {
                LPWSTR pszFilePath = NULL;
                psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &pszFilePath);
                char mraPath[MAX_PATH];
                wcstombs(mraPath, pszFilePath, MAX_PATH);
                
                if (!get_crc_from_mra(mraPath, &target_crc)) {
                    MessageBoxA(NULL, "El archivo no contiene la l\xEDnea \"beta.bin\" con su CRC\no el CRC encontrado no es v\xE1lido.", "Error", MB_ICONERROR);
                    CoTaskMemFree(pszFilePath); psi->lpVtbl->Release(psi); pfd->lpVtbl->Release(pfd);
                    CoUninitialize();
					return 0;
                }
                CoTaskMemFree(pszFilePath);
                psi->lpVtbl->Release(psi);
            }
        }
        pfd->lpVtbl->Release(pfd);
    }
    if (target_crc == 0) {
		CoUninitialize();
		return 0;
	}

	// --- IFileOpenDialog para el ZIP ---
	hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL, &IID_IFileOpenDialog, (void**)&pfd);
    char szSavePath[MAX_PATH] = {0};

	if (SUCCEEDED(hr)) {
		FILEOPENDIALOGOPTIONS dwOptions;
		pfd->lpVtbl->GetOptions(pfd, &dwOptions);
		pfd->lpVtbl->SetOptions(pfd, dwOptions | FOS_PICKFOLDERS);
		pfd->lpVtbl->SetTitle(pfd, L"¿D\xF3nde guardar jtbeta.zip?");
		
		hr = pfd->lpVtbl->Show(pfd, NULL);
		if (SUCCEEDED(hr)) {
			IShellItem *psi = NULL;
			hr = pfd->lpVtbl->GetResult(pfd, &psi);
			if (SUCCEEDED(hr)) {
				LPWSTR pszFolderPath = NULL;
				psi->lpVtbl->GetDisplayName(psi, SIGDN_FILESYSPATH, &pszFolderPath);
				wcstombs(szSavePath, pszFolderPath, MAX_PATH);
				strcat(szSavePath, "\\jtbeta.zip"); // <--- Aquí se asegura el nombre
				CoTaskMemFree(pszFolderPath);
				psi->lpVtbl->Release(psi);
			}
		}
		pfd->lpVtbl->Release(pfd);
	}

    if (strlen(szSavePath) == 0) {
		CoUninitialize();
		return 0;
	}

	DWORD dwAttrib = GetFileAttributesA(szSavePath);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        int response = MessageBoxA(NULL, 
            "El archivo 'jtbeta.zip' ya existe en esta ubicaci\xF3n.\n\nDeseas reemplazarlo?", 
            "Confirmar sobrescritura", 
            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        
        if (response == IDNO) {
            CoUninitialize();
            return 0;
        }
    }

	// --- Calcular bytes ---
	init_tables();
    uint8_t data[4];
    solve_crc32(target_crc, data);

    FILE *f = fopen(szSavePath, "wb");
    if (!f) {
        MessageBoxA(NULL, "No se pudo crear el archivo 'jtbeta.zip'.", "Error", MB_ICONERROR);
    } else {

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

    char finalMsg[256];
    sprintf(finalMsg, "Archivo 'jtbeta.zip' generado con \xE9xito.\nCRC procesado: %08X\nBytes: %02X %02X %02X %02X", target_crc, data[0], data[1], data[2], data[3]);
    MessageBoxA(NULL, finalMsg, "Finalizado", MB_OK | MB_ICONINFORMATION);
	}

	CoUninitialize();
    return 0;
}
