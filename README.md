#  Device mapper proxy
DMP -- модуль ядра Linux для создания виртуальных блочных устройств поверх существующих.
 
## Сборка
Для сборки модуля нужно загрузить исходный код, выполнить команду make в директории с ним.
```
make
```
Будет создан объектный файл модуля dmp.ko .
## Установка
Все нижеперечисленные действия нужно выполнять с правами суперпользователя. Для этого можно воспользоваться командами sudo, su.
Установка модуля. "module_path"  - путь до файла модуля.
```
insmod module_path/dmp.ko
```
## Тестирование
Тестирование проводилось для Ubuntu 22.04.3 LTS.
### Тестирование с использованием dm-zero
1. Cоздание блочного устройства
```
dmsetup create zero1 --table "0 20000 zero"
```
2. Создание dmp-устройства
```
dmsetup create dmp1 --table "0 20000 dmp /dev/mapper/zero1 "
```
3. Проверка
```
ls -al /dev/mapper/*
```
> lrwxrwxrwx 1 root root 7 фев 7 20:49 dmp1 -> ../dm-1
> lrwxrwxrwx 1 root root 7 фев 7 20:49 zero1 -> ../dm-0
4. Выполнение записи, чтения
```
dd if=/dev/zero of=/dev/mapper/dmp1 bs=4096 count=10
```
>1+0 records in
>1+0 records out
>4096 (4,1 kB, 4,0 KiB) bytes copied, 0,00304787 s, 1,3 MB/s
```
dd if=/dev/mapper/dmp1 of=/dev/null bs=4096 count=10
```
>1+0 records in
>1+0 records out
>4096 (4,1 kB, 4,0 KiB) bytes copied, 0,000357671 s, 11,5 MB/s
5. Просмотр статистики
```
cat /sys/module/dmp/stat/volume
```
> read:
>  reqs: 122
>  avg size: 4096
> write:
>  reqs: 1
>  avg size: 4096
> total:
>  reqs: 123
>  avg size: 4096
### Тестирование с использованием dm-linear
1. Создание loop-устройства из пустого файла
```
dd if=/dev/zero of=testfile bs=1024 count=20
losetup /dev/loop10 testfile
```
2. Cоздание dm-linear устройства
```
dmsetup create lin1 --table "0 40 linear /dev/loop10 0"
```
3. Создание dmp-устройства
```
dmsetup create dmp1 --table "0 20 dmp /dev/mapper/lin1"
```
4. Проверка
```
ls -al /dev/mapper/*
```
> lrwxrwxrwx 1 root root 7 фев 7 20:39 dmp1 -> ../dm-1
> lrwxrwxrwx 1 root root 7 фев 7 20:39 lin1 -> ../dm-0
5. Выполнение записи, чтения
```
dd if=/dev/urandom of=/dev/mapper/dmp1 bs=512 count=10
```
>10+0 records in
>10+0 records out
>5120 (5,1 kB, 5,0 KiB) bytes copied, 0,0104916 s, 488 kB/s
```
dd if=/dev/mapper/dmp1 of=/dev/null bs=512 count=10
```
>10+0 records in
>10+0 records out
>5120 (5,1 kB, 5,0 KiB) bytes copied, 0,000349746 s, 14,6 MB/s
6. Просмотр статистики
```
cat /sys/module/dmp/stat/volume
```
> read:
>  reqs: 9
>  avg size: 3413
> write:
>  reqs: 3
>  avg size: 2048
> total:
>  reqs: 12
>  avg size: 2731
7. Просмотр структуры блочных устройств
```
dmsetup ls --tree -o blkdevname
```
> dmp1 <dm-1> (253:1)
>   -lin1 <dm-0> (253:0)
>      - <loop10> (7:10)
