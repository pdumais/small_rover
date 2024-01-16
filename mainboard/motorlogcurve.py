import math

# I got these numbers just from trial and error by generating graphs on https://www.desmos.com/calculator
# b = 1.02
# k = 29.1
b = 1.01
k = 7.55  # to reach 4096 instead

lst = [round(k * math.log(x + 1, b)) for x in range(0, 255)]
lst[0] = 0
# lst.append(8191)
lst.append(4095)

print(len(lst))
print(lst)
