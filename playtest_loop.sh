#!/bin/sh

# loops editing, compilation, and running game
# very useful for rapidly testing small alterations
# control-c out of game to exit loop

while [ true ]; do
	$EDITOR mines.c
	make
	./mines
done

