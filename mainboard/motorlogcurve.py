import math

# I got these numbers just from trial and error by generating graphs on https://www.desmos.com/calculator
b = 1.02
k = 23.4

lst = [round(k * math.log((x + 1) * 4, b)) for x in range(0, 255)]
lst[0] = 0
lst.append(8191)
# lst.append(4095)

print(len(lst))
print(lst)
