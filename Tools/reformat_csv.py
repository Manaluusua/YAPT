import csv
import argparse

def reformat_csv(input_file, output_file, chunk_size, delim):
    with open(input_file, 'r', newline='', encoding='utf-8') as csvfile:
        reader = csv.reader(csvfile)
        data = list(reader)  
        
        if not data:
            print("The input CSV file is empty.")
            return
        
        # Transpose columns to rows
        transposed_data = list(zip(*data))
        
        with open(output_file, 'w', newline='', encoding='utf-8') as csvout:
            writer = csv.writer(csvout, delimiter=delim)
            
            for row in transposed_data:
                
                for i in range(0, len(row), chunk_size):
                    writer.writerow(f'{item},' for item in row[i:i + chunk_size])#writer.writerow(row[i:i + chunk_size])
                writer.writerow('')
                writer.writerow('')
    
    print(f"Reformatted CSV saved to {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Reformat CSV columns into multiple lines.")
    parser.add_argument("input_file", help="Path to the input CSV file")
    parser.add_argument("output_file", help="Path to the output CSV file")
    parser.add_argument("--chunk_size", type=int, default=3, help="Number of elements per row in output (default: 3)")
    parser.add_argument("--delimiter", type=str, default=" ", help="Number of elements per row in output (default: 3)")
    
    args = parser.parse_args()
    
    reformat_csv(args.input_file, args.output_file, args.chunk_size, args.delimiter)