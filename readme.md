# disk_utilization

A program that prints continuous disk utilization values

## Usage

```
diskutil [-h] [-f] [-i <float>] [-c <int>] [-p <string>] <disk>
	 --human    | -h   : Output in percent
	 --fraction | -f   : When human, also print a fraction
	 --interval | -i   : Update interval in seconds (default 1.0)
	 --count    | -c   : # of output lines (default -1 = inf)
	 --prefix   | -p   : Prefix string to print before each value
	 --suffix   | -s   : Suffix string to print after each value (def. \n)
```

Simple Example:
```sh
diskutil -h sda
```
May output every second:
```
2%
0%
0%
...
```

Advanced Example:
```sh
diskutil -hf -i 3.1415 -c 5 --prefix="Usage: " sda
```
May output every ~PI seconds:
```
Usage: 0.00%
Usage: 0.86%
Usage: 0.00%
Usage: 1.78%
Usage: 42.57%
```

## Building

```sh
make
```
