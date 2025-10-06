import pandas as pd
import matplotlib.pyplot as plt
import sys

# Replace 'data.csv' with your file name
# Replace 'column_name' with the name of the column you want to plot
file_path = sys.argv[1]
column_name = 'access_times'

# Load the data
data = pd.read_csv(file_path)
print(data)

# Plot the histogram
plt.hist(data[column_name].dropna(), bins=30, edgecolor='black')
plt.title(f'Histogram of {column_name}')
plt.xlabel(column_name)
plt.ylabel('Frequency')
plt.show()
