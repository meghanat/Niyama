
#RUN 3 - cgroups

sudo rm -rf cgroups_only
mkdir cgroups_only
mkdir cgroups_only/logs

rm time_taken.txt
date >> time_taken.txt

gcc cgroups_job.c -o emulate_job -lpthread -lm
rm wl_resp.txt
touch wl_resp.txt
gcc controller.c -o controller
sudo ./controller 0 &

echo $!

wait $!

sudo killall controller
sudo killall emulate_job

sudo killall controller
sudo killall emulate_job

date >> time_taken.txt

cp wl_resp.txt cgroups_only/response_time.csv
cp time_taken.txt cgroups_only/time_taken.txt
cp output.txt cgroups_only/util_log.txt