gpx-sanitize
============

A simple GPX "sanitizer": it tries to sort &lt;trkpt/&gt;s elements from &lt;trakseg/&gt; by proximity.

Why ?
=====

I wanted to draw a kind of art poster from the OpenStreetMap contributed GPS traces.
But these ones can be "anonymized", i.e. the points of the track segments are supposed 
to be shuffled.

I first worked on a ruby implementation, but it was way too long to execute (~5 minutes 
on my computer to reorder 4702 points), then I reimplemented it in C.

And then ?
===========

It became a kind of benchmark between good ol' compiled languages and fancy-trendy 
interpreted ones. Conclusion ? If you're able to write C without taking too much time
or having headaches, you should definitely consider using C.

Anyway, here is the result:
![](https://pbs.twimg.com/media/Bf5pLlrCIAAskBc.png:large)
