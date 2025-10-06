import pandas as pd
import matplotlib.pyplot as plt
import sys

def plot_csv(file_path):
    try:
        # Read the CSV (no header)
        data = pd.read_csv(file_path, header=None)
        
        if data.shape[1] != 2:
            print("Error: CSV must have exactly two columns.")
            return

        x = data[0]
        y = data[1]

        plt.figure(figsize=(8, 5))
        plt.plot(x, y, marker='o')
        plt.title("Plot from CSV")
        plt.xlabel("Number of Branches")
        plt.ylabel("Average Access Time Per Branch")
        plt.grid(True)
        plt.tight_layout()
        plt.show()

    except Exception as e:
        print(f"Failed to plot CSV: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python plot_csv.py <path_to_csv>")
    else:
        plot_csv(sys.argv[1])
