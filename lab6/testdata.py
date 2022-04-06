import random
import sys


class NoActionDefinedError(Exception):
    pass


def test(filename="output.txt"):
    with open(filename, "r") as fh:
        my_data = [ int(i) for i in fh.readline().split(",")]
    test_data = sorted(my_data)
    return test_data == my_data


def fill_random(filename="input.txt"):
    with open(filename, "w+") as fh:
        data = [random.randint(-10000, 10000) for _ in range(1_000_000)]
        fh.write(", ".join([str(d) for d in data]))


def main():
    if len(sys.argv) >= 2:
        if sys.argv[1] == "fill":
            if len(sys.argv) == 3:
                fill_random(sys.argv[2])
            else:
                fill_random()
        elif sys.argv[1] == "test":
            if len(sys.argv) == 3:
                if test(sys.argv[2]):
                    print("Test succeeded.")
                    exit(0)
                print("Test failed.")
                exit(1)
            if test():
                print("Test succeeded.")
                exit(0)
            print("Test failed.")
            exit(1)
    
    else:
        raise NoActionDefinedError("No action defined.")

if __name__ == "__main__":
    main()