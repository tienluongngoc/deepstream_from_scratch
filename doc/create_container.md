
# Tạo môi trường
```
git clone https://github.com/tienln4/deepstream_from_scratch.git
cd deepstream_from_scratch

docker run -it --name your_container_name --gpus all --network host -v $PWD:/deepstream nvcr.io/nvidia/deepstream:5.1-21.02-triton /bin/bash
apt update
apt install ssh
apt install vim
```
- Cài đạt ssh server cho container
```
vim /etc/ssh/sshd_config
```

- Sửa: 
```
Port 9999
sửa PermitRootLogin yes
```

- start ssh service
```
service ssh start
```
