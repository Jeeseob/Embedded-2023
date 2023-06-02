# Embedded System Software HW 2

</br>

서강대학교 2023-1학기
임베디드 시스템 소프트웨어 과제 2입니다.

해당 과제는 Embedded board에 연결된 FPGA device 중 4가지를 
timer 기능으로 사용할 수 있도록 하는 device driver를 구현하는 과제 입니다.

본 프로젝트는 Device driver, Test pplication,    
총 2가지 software로 구성되어 있습니다.

</br>

## Driver Information

#### Driver name : dev_driver    
#### Major number : 242         

</br>

## install corss compiler
```
$ cd ~/Downloads
$ sudomkdir/opt/toolchains
$ wgethttp://sources.buildroot.net/toolchain-external-codesourcery-arm/arm-2014.05-29-arm-
none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2
$ sudotar-jxvfarm-2014.05-29-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2-C
/opt/toolchains
```


## Compile and Run

#### Host
```
# compile application
$ cd /.../app
$ make
$ adb push test_dev_driver /data/local/tmp

# compile driver
$ cd /.../module
$ make
$ adp push dev_driver.ko /data/local/tmp

```

#### Machine(minicom)
```
# Set device driver
$ cd /data/local/tmp
$ mknod /dev/dev_driver c 242 0
$ insmod dev_driver.ko

# Run test application
$ ./test_dev_driver [1-100] [1-100] [0001 - 8000]
```

</br>

해당 코드는 아래의 Embedded board에서 동작합니다.

![스크린샷 2023-04-30 오후 8 17 40](https://user-images.githubusercontent.com/81093419/235350087-e6c53355-0a30-44b4-a195-affa66bc6b58.png)