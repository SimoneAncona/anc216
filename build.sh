rm -r assembler/build/
rm -r cardreader/build/
cd assembler/ && ./build.sh && cd ..
cd cardreader/ && ./build.sh && cd ..