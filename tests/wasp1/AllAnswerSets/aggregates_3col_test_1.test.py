input = """
node(1..5).
col(blue).
col(green).
col(red).

color(N,C) v ncolor(N,C) :- node(N), col(C).
:- color(X,C), color(Y,C), link(X,Y).

:- color(N,C), color(N,C1), C!=C1.
:- not #count{ N: color(N,C) } > 4.

link(1,2).
link(1,3).
link(2,3).
link(3,4).
link(4,5).
"""

output = """
{col(blue), col(green), col(red), color(1,blue), color(2,green), color(3,red), color(4,blue), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,green), color(3,red), color(4,blue), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,green), color(3,red), color(4,green), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,blue), ncolor(4,red), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,green), color(3,red), color(4,green), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,blue), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,red), color(3,green), color(4,blue), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,blue), ncolor(3,red), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,red), color(3,green), color(4,blue), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,blue), ncolor(3,red), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,red), color(3,green), color(4,red), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,blue), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,blue), color(2,red), color(3,green), color(4,red), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,green), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,blue), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,blue), color(3,red), color(4,blue), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,blue), color(3,red), color(4,blue), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,blue), color(3,red), color(4,green), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,blue), ncolor(4,red), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,blue), color(3,red), color(4,green), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,green), ncolor(4,blue), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,red), color(3,blue), color(4,green), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,red), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,red), color(3,blue), color(4,green), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,red), color(3,blue), color(4,red), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,green), color(2,red), color(3,blue), color(4,red), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,red), ncolor(2,blue), ncolor(2,green), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,blue), color(3,green), color(4,blue), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,red), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,blue), color(3,green), color(4,blue), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,red), ncolor(4,green), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,blue), color(3,green), color(4,red), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,blue), color(3,green), color(4,red), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,green), ncolor(2,red), ncolor(3,blue), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,green), color(3,blue), color(4,green), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,blue), ncolor(2,red), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,red), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,green), color(3,blue), color(4,green), color(5,red), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,blue), ncolor(2,red), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,red), ncolor(5,blue), ncolor(5,green), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,green), color(3,blue), color(4,red), color(5,blue), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,blue), ncolor(2,red), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,green), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
{col(blue), col(green), col(red), color(1,red), color(2,green), color(3,blue), color(4,red), color(5,green), link(1,2), link(1,3), link(2,3), link(3,4), link(4,5), ncolor(1,blue), ncolor(1,green), ncolor(2,blue), ncolor(2,red), ncolor(3,green), ncolor(3,red), ncolor(4,blue), ncolor(4,green), ncolor(5,blue), ncolor(5,red), node(1), node(2), node(3), node(4), node(5)}
"""
