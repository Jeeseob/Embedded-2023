# Embedded System Software 2023 HW1

서강대학교 2023-1학기
임베디드 시스템 소프트웨어 과제 1입니다.

해당 과제는 Embedded board를 사용한 key value store 구현입니다.

총 3개의 process로 구현되어 있으며, 
I/O device의 contorl은 thread로 분리하여 실행되도록 구현하였습니다.

#### install corss compiler
```
$ cd ~/Downloads
$ sudomkdir/opt/toolchains
$ wgethttp://sources.buildroot.net/toolchain-external-codesourcery-arm/arm-2014.05-29-arm-
none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2
$ sudotar-jxvfarm-2014.05-29-arm-none-linux-gnueabi-i686-pc-linux-gnu.tar.bz2-C
/opt/toolchains
```

#### Compile and Run
```
$ arm-none-linux-gnueabi-gcc -static -pthread -o main.o main.c
$ ./main.o
```

해당 코드는 아래의 embedded board에서 동작합니다.

![스크린샷 2023-04-30 오후 8 17 40](https://user-images.githubusercontent.com/81093419/235350087-e6c53355-0a30-44b4-a195-affa66bc6b58.png)