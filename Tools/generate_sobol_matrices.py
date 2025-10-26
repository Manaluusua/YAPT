import numpy as np
import argparse

def check_file_header(line):
    first_row = list(line.split())
    if(first_row[0] != 'd' or first_row[1] != 's' or first_row[2] != 'a' or first_row[3] != 'm_i'):
        print(f"unexpected first row (should be d s a m_i, got {first_row}")
        return False
        
    return True

def load_sobol_initial_directions_file(filepath):
    params = []
    first_row = True
    with open(filepath, 'r') as file:
        for line in file:
            if line.strip().startswith('#') or not line.strip():
                continue

            if(first_row):
                if(not check_file_header(line)):
                    return None
                first_row = False
                continue
        
            parts = list(map(int, line.split()))
            dim = parts[0]
            s = parts[1]
            
            binary_str = bin(parts[2])[2:]
            a = [int(bit) for bit in binary_str]
            
            #prebend zeroes omitted to not have to deal with it later
            if(len(a) < s):
                a = np.concatenate((np.zeros(s - len(a), dtype=np.int8), a), dtype=np.int8)
            
            m = parts[3: 3 + s]

            params.append((dim, s, a, m))
    return params



def build_direction_numbers(s, a, m, L):
    # Extend m_j recursively for j > s
    mj = m.copy()
    for j in range(s, L):
        new_m = mj[j - s] << s
        for k in range(1, s):
            if a[k - 1] == 1:
                new_m ^= mj[j - k] << k
        mj.append(new_m)
    return mj

def getIntegerType(precision):
    precisionType = np.uint8
    if(precision > 8 and precision <= 16):
        precisionType = np.uint16
    elif (precision > 16 and precision <= 32):
        precisionType = np.uint32
    elif (precision > 32):
        precisionType = np.uint64
    return precisionType

def getCIntegerTypeAndLiteral(precision):
    if precision <= 8:
        return ("uint8_t", "")
    elif precision > 8 and precision <= 16:
        return ("uint16_t", "")
    elif precision > 16 and precision <= 32:
        return ("uint32_t", "u")
    elif precision > 32:
        return ("uint64_t", "ull")

def generator_matrix(m, L):
    precisionType = getIntegerType(L)
    
    C = np.zeros((L), dtype=precisionType)
    for j in range(L):
        mj = m[j]
        for k in range(L):
            C[j] = mj #(mj >> (L - 1 - k)) & 1
    return C

def write_results(mat_arr, bits_count, output_file):
    file_begin = ["#pragma once", "#include <cstdint>", "namespace YAPT", "{"]

    columns = min(8, bits_count)
    
    ctype, cliteral = getCIntegerTypeAndLiteral(bits_count)
    indent = "\t"
    line_break = lambda file: file.write(f"\n{indent}{indent}")

    f_num = "{:02x}"
    if(bits_count > 8 and bits_count <= 16):
        f_num = "{:04x}"
    elif(bits_count > 16 and bits_count <= 32):
        f_num = "{:08x}"
    elif(bits_count > 32):
        f_num = "{:016x}"
       
    with open(output_file, 'w') as file:
        for line in file_begin:
            file.write(line + "\n")
            
        file.write(f"{indent}constexpr size_t SOBOL_MATRIX_ROW_COUNT={bits_count};\n")
        file.write(f"{indent}constexpr size_t SOBOL_MATRIX_COUNT={len(mat_arr)};\n")
        file.write(f"{indent}constexpr {ctype} SOBOL_MATRICES[SOBOL_MATRIX_COUNT*SOBOL_MATRIX_ROW_COUNT] =\n{indent}{{")
        
        columns_written = 0
        line_break(file)
        
        for mat in mat_arr:
            for v in mat:
                file.write(f"0x{f_num.format(v)}{cliteral},")
                
                columns_written = columns_written + 1
                if columns_written == columns:
                    columns_written = 0
                    line_break(file)
                    
            line_break(file)
            
        file.write(f"\n{indent}}};\n}}")
            

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate sobol generator matrices")
    parser.add_argument("--output_file", default="SobolMatrices.h", help="Path to the output file")
    parser.add_argument("--directions_file", default="new-joe-kuo-6.21201", help="Path to a file with initial direction numbers (assumes joe & kuo file format)")
    parser.add_argument("--bits", type=int, default=32, help="bit precision, defaults to 32 bit")
    parser.add_argument("--dimensions", type=int, default=256, help="Number of dimensions to produce")
    
    args = parser.parse_args()
    
    bit_count = args.bits
    if(bit_count > 64):
        bit_count = 64
        print("Currently max bitdepth supported is 64, clamping to 64.")

    directions = load_sobol_initial_directions_file(args.directions_file)
    if(directions == None):
        print("Malformed input file")
        sys.exit()
   
    if(args.dimensions >= len(directions)):
        print(f"asked to produce {args.dimensions} dimensions but directions file only contains initial direction numbers for {len(directions)-1} dimensions")
        sys.exit()
    mat_arr = []
    for i in range(args.dimensions):
        dim, s, a, m = directions[i]
        mj = build_direction_numbers(s, a, m, bit_count)
        c = generator_matrix(mj, bit_count)
        mat_arr.append(c)
        #print(C)
        #print(f"dim {dim} s {s} a {a} m {m}")
    write_results(mat_arr, bit_count, args.output_file)