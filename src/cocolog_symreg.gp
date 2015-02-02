set key right top;

set term pngcairo size 1280, 800 font "mono,small"
set size 1, 1

set key font ",8"
set grid ytics lt 0 lw 1 lc rgb "#bbbbbb"
set grid xtics lt 0 lw 1 lc rgb "#bbbbbb"

set style fill transparent solid 0.5

set xlabel "x"
set xtics 1
set ylabel "f(x)"

set output "cocolog/plot-symreg.png"
plot "cocolog/input.dta" every ::1 using 1:2 title "input", \
     "cocolog/best.dta" every ::1 using 1:2 title "best"



####

set datafile separator ","
set key autotitle columnhead

set key font ",8"
set grid ytics lt 0 lw 1 lc rgb "#bbbbbb"
set grid y2tics lt 0 lw 1 lc rgb "#bbbbbb"
set grid xtics lt 0 lw 1 lc rgb "#bbbbbb"

set style fill transparent solid 0.5

set xlabel "Generace"
set ylabel "Fitness"
set xtics auto
set ytics 10
set yrange [0:200]
#set xrange [0:30000]

set y2label "Prediktor [%]"
set y2tics 5
set y2range [0:100]

set output "cocolog/plot-fitness_pred.png"
plot "cocolog/cgp_history.csv" using 1:2 with steps, \
     '' using 1:3 with steps, \
     '' using 1:5 with steps, \
     '' using ($1):(100*$8/201) with steps lc 5 axes x1y2, \
     '' using ($1):(100*$7/201) with steps lc 4 lw 2 axes x1y2
