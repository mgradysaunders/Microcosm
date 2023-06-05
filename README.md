# Microcosm

> mi·cro·cosm (noun)
> 
> a community, place, or situation regarded as encapsulating in miniature the characteristic qualities or features of something much larger.

Hello :wave: and welcome! Microcosm is an extensible collection of modern C++ libraries intended to be, well, a *microcosm* for [topics in computer graphics](#topics). While everything is all in one repo, it is not one library. Rather, it is a bunch of libraries written side by side with some sensible level of interdepence, all sharing a template-heavy header-only core.

Microcosm prioritizes three philosophical design pillars:

1. To be **clear and instructive**.
   Use understandable names. Write meaningful comments. Stop and think about language features and design patterns on occasion.

2. To be **minimal**.
   Eliminate redundancy by identifying and refactoring meaningful operations. Do not sacrifice simplicity on the altar of optimization unless completely necessary.

3. To be **as self contained as possible**.
   Only pull in external dependencies when there is a very good reason to do so. Even then, prefer implementations that can be (legally) embedded into the source to keep the build simple.

