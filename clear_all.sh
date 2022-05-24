#sudo killall sh

sudo killall controller
sudo killall emulate_job
sudo killall controller
sudo killall emulate_job
sudo killall controller
sudo killall emulate_job

sudo rm -rf cfs_only
sudo rm -rf cgroups_only
sudo rm -rf deadline_only
sudo rm output.txt
sudo rm nohup.out

sudo killall sh
