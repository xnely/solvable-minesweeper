# Solvable Mines

A fork of [GNOME-mines](https://github.com/GNOME/gnome-mines) that never forces you to take a chance \
Puzzle games are about solving puzzles and finding patterns, not rolling the dice.

## Explanation

Mines often allows for unsolvable boards like this:

![Unsolvable-state](/images/solvable_un_2.png)

On large board sizes (like 30x16), **~80%** of games are **unsolvable**, giving the game a large element of luck. \
Solvable Mines removes this element, so you will only ever lose due to your own mistakes.

### Note

While Solvable Mines is solvable, it is not neccessarily easy. \
In this example, we know 97/99 mines have been found

![Flag](/images/flag.png)

You can find the exact position of the remaining two mines without clearing any more squares.

![Solvable-state](/images/solvable_2.png)

If you see a situation like this early in the game, come back later when you have more of the board cleared. \
The number of mines remaining may be critical for clearing the section.


### How it works

When you first click on the board, mines are randomly placed around the board (except for where you clicked, ofcourse). \
The board is copied and an AI plays it with the same rules as the player. \
If the AI ever gets stuck, then the board is unsolvable. \
The AI will then break the rules and modify the board to try to make it solvable. If it fails, a new board is generated. \
This slightly increases the likelyhood of mines being in the spaces the furthest from where you clicked, but prevents the performance hit of generating and testing a dozen boards to get a single playable game.

## Building

clone the repo
```bash
~$ cd solvable-mines
~/solvable-mines$ meson build
~/solvable-mines/build$ cd build
~/solvable-mines/build$ ninja
```
To install system-wide:
```bash
~/solvable-mines/build$ sudo ninja install
```
Or run locally:
```bash
~/solvable-mines/build$ ./src/solvable-mines
```