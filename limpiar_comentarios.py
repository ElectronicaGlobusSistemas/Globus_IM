#!/usr/bin/env python3
"""
Limpia líneas vacías consecutivas en Buffers_Cashless.cpp
"""

def limpiar_lineas_vacias(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    output = []
    lineas_vacias_consecutivas = 0

    for line in lines:
        if line.strip() == '':
            lineas_vacias_consecutivas += 1
            if lineas_vacias_consecutivas <= 2:  # Mantener máximo 2 líneas vacías
                output.append(line)
        else:
            lineas_vacias_consecutivas = 0
            output.append(line)

    # Escribir archivo limpio
    with open(file_path, 'w', encoding='utf-8') as f:
        f.writelines(output)

    # Contar líneas finales
    return len(output)

if __name__ == "__main__":
    archivo = r"C:\Users\USUARIO\Documents\Desktop\Nueva carpeta\01\RUTA-RD\02-IA\Globus_IM\src\Buffers_Cashless.cpp"
    lineas_finales = limpiar_lineas_vacias(archivo)
    print(f"Líneas finales después de limpiar espacios: {lineas_finales}")
