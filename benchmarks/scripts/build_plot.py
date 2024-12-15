import matplotlib.pyplot as plt
import csv
import sys

Count = []
file = sys.argv[1]
comment = sys.argv[2]

InterstingCollumns = (3, 6, 9, 10, 11, 14, 17)
CollumnsName = []
Collumns = ([], [], [], [], [], [], [])


with open(f'{file}.csv', 'r') as datafile:
    plotting = csv.reader(datafile, delimiter=';')

    header = next(plotting)
    for collumn in InterstingCollumns:
        CollumnsName.append(header[collumn])

    print('Header: {}'.format(header))
    for ROWS in plotting:
        # print('Row: {}'.format(ROWS))
        Count.append(float(ROWS[0]))
        idx = 0
        for collumn in InterstingCollumns:
            Collumns[idx].append(float(ROWS[collumn]))
            idx += 1


for collumn in Collumns:
    plt.plot(Count, collumn)
plt.title(comment)
plt.xlabel('Amount of entities')
plt.ylabel('Time [ms]')
# plt.xscale('log')
plt.grid()
plt.legend(CollumnsName)
# plt.show()
plt.savefig(f'{file}.png', dpi=200)