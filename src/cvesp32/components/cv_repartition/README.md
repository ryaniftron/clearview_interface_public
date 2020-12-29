Goal: Repartition

https://unix.stackexchange.com/questions/176111/how-to-dump-a-binary-file-as-a-c-c-string-literal


python ~/Development/esp/esp-idf/components/partition_table/gen_esp32part.py partitions.csv binary_partitions.bin

xxd -i -u -c 32 binary_partitions.bin

