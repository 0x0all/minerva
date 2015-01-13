import sys

def parseSecond(i):
    hour, minute, second = map(float, i.split(':'))
    return hour * 3600 + minute * 60 + second

if __name__ == '__main__':
    lines = sys.stdin.readlines()
    for i in range(len(lines) - 1):
        diff = parseSecond(lines[i + 1].split()[1]) - parseSecond(lines[i].split()[1])
        print '%f %s' % (diff, lines[i].strip())
