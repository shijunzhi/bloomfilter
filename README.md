# The bloomfilter of redis module
A redis module to extend redis to support bloomfilter.

## Support commands

### bloomfilter.create

```
bloomfilter.create <key name> <data set size> <error rate>
```
Create a bloomfilter

### bloomfilter.add

```
bloomfilter.add <key name> <value1> ...
```
Add one or more data in filter

### bloomfilter.check

```
bloomfilter.check <key name> <value>
```
Check if given value is in data set

### bloomfilter.destroy

```
bloomfilter.destroy <key name> ...
```
Delete one or more given bloomfilter