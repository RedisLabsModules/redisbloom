<img src="images/logo.svg" alt="logo" width="200"/>

# RedisBloom: Probabilistic Data Structures for Redis
[![Forum](https://img.shields.io/badge/Forum-RedisBloom-blue)](https://forum.redislabs.com/c/modules/redisbloom)
[![Discord](https://img.shields.io/discord/697882427875393627?style=flat-square)](https://discord.gg/wXhwjCQ)

RedisBloom adds four probabilistc data structures to Redis: a scalable **Bloom filter**,  a **cuckoo filter**, a **count-min sketch**, and a **top-k**. These data structures trade perfect accuracy for extreme memory efficiency, so they're especially useful for big data and streaming applications.

**Bloom and cuckoo filters** are used to determine, with a high degree of certainty, whether an element is a member of a set.

A **count-min sketch** is generally used to determine the frequency of events in a stream. You can query the count-min sketch get an estimate of the frequency of any given event.

A **top-k** maintains a list of _k_ most frequently seen items.

## Quick start guide
1. [Quick Start](Quick_Start.md)
1. [Command references](#command-references)
1. [Client libraries](#client-libraries)
1. [References](#references)
1. [License](#license)

## Command references
Detailed command references for each data structure:

* [Bloom Filter](Bloom_Commands.md)
* [Cuckoo Filter](Cuckoo_Commands.md)
* [Count-Min Sketch](CountMinSketch_Commands.md)
* [Top-K](TopK_Commands.md)
* [T-Digest](TDigest_Commands.md)

## Bloom vs. Cuckoo
Bloom filters typically exhibit better performance and scalability when inserting
items (so if you're often adding items to your dataset, then a Bloom filter may be ideal).
Cuckoo filters are quicker on check operations and also allow deletions.

## Client libraries
See each driver's README for details and documentation.

| Project | Language | License | Author | URL |
| ------- | -------- | ------- | ------ | --- |
| redisbloom-py | Python | BSD | [Redis Labs](https://redislabs.com) | [GitHub](https://github.com/RedisBloom/redisbloom-py) |
| JRedisBloom | Java | BSD | [Redis Labs](https://redislabs.com) | [GitHub](https://github.com/RedisBloom/JRedisBloom) |
| redisbloom-go | Golang | BSD | [Redis Labs](https://redislabs.com) | [GitHub](https://github.com/RedisBloom/redisbloom-go) |
| rebloom | JavaScript | MIT | [Albert Team](https://cvitae.now.sh/) | [GitHub](https://github.com/albert-team/rebloom) |
| phpredis-bloom | PHP | MIT | [Rafa Campoy](https://github.com/averias) | [GitHub](https://github.com/averias/phpredis-bloom) |
| phpRebloom | PHP | MIT | [Alessandro Balasco](https://github.com/palicao) | [GitHub](https://github.com/palicao/phpRebloom) |
| redis-modules-sdk | TypeScript | BSD-3-Clause | [Dani Tseitlin](https://github.com/danitseitlin) | [GitHub](https://github.com/danitseitlin/redis-modules-sdk) |
| redis-modules-java | Java | Apache License 2.0 | [dengliming](https://github.com/dengliming) | [GitHub](https://github.com/dengliming/redis-modules-java) |

## References
### Webinars
1. [Probabilistic Data Structures - The most useful thing in Redis you probably aren't using](https://youtu.be/dq-0xagF7v8?t=102)

### Blog posts
1. [RedisBloom Quick Start Tutorial](https://docs.redislabs.com/latest/modules/redisbloom/redisbloom-quickstart/)
1. [Developing with Bloom Filters](https://docs.redislabs.com/latest/modules/redisbloom/)
1. [RedisBloom on Redis Enterprise](https://redislabs.com/redis-enterprise/redis-bloom/)
1. [Probably and No: Redis, RedisBloom, and Bloom Filters](https://redislabs.com/blog/redis-redisbloom-bloom-filters/)
1. [RedisBloom – Bloom Filter Datatype for Redis](https://redislabs.com/blog/rebloom-bloom-filter-datatype-redis/)

## Mailing List / Forum
Got questions? Feel free to ask at the [RedisBloom forum](https://forum.redislabs.com/c/modules/redisbloom).

## License
RedisBloom is licensed under the [Redis Source Available License Agreement](LICENSE)
