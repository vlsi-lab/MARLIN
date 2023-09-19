mkdir -p ./saif
mkdir -p ./report

# setup Synopsys environment

for ((appr_level = 0; appr_level < 256; appr_level++))
do
  for ((a_w = 9; a_w <= 9; a_w++))
  do
    for ((b_w = 9; b_w <= 9; b_w++))
    do
        vcd2saif -input ./vcd/approx_mul_${a_w}x${b_w}_${appr_level}.vcd -output ./saif/approx_mul_${a_w}x${b_w}_${appr_level}.saif
        dc_shell-xg-t -no_gui -f ./power.scr -x "set my_list {$a_w $b_w $appr_level}"
    done
  done
done 
