# The bloomfilter of redis module
A redis module to extend redis to support bloomfilter.

## Support operations

### Create one bloomfilter

```
bloomfilter.create <key name> <data set size> <error rate>
```

### Add one or more data to filter

```
bloomfilter.add <key name> <value1> ...
```

### Check if given value is in data set

```
bloomfilter.check <key name> <value>
```

### Delete one or more bloomfilters

```
del <key name>
```

## RDB

Support RDB save and load from RDB file

## AOF

When AOF rewrite, one bloomfilter will be rewrite by one internal usage command **bloomfilter.set**.

## Replication

Support
