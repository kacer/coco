reset
set term postscript eps color enhanced size 5cm,5cm
set output "ruleta2.eps"

set size square
set style fill solid 1.0 border -1

set object 1 circle at screen 0.5,0.5 size \
  screen 0.45 arc [0     :120] fillcolor rgb "red" front
set object 2 circle at screen 0.5,0.5 size \
  screen 0.45 arc [120:216] fillcolor rgb "forest-green" front
set object 4 circle at screen 0.5,0.5 size \
  screen 0.45 arc [216:288] fillcolor rgb "dark-magenta" front
set object 5 circle at screen 0.5,0.5 size \
  screen 0.45 arc [288:336] fillcolor rgb "blue" front
set object 6 circle at screen 0.5,0.5 size \
  screen 0.45 arc [336:360] fillcolor rgb "orange" front

#plot a white line, i.e., plot nothing
unset border
unset tics
unset key
plot x with lines lc rgb "#ffffff"
set output

