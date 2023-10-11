# 변수 정의
TARGET = kernel.elf  # Makefile이 만들어내는 최종 생성물
OBJS = main.o	# TARGET을 만드는 데 필요한 오브젝트 파일

# 컴파일 옵션
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
	-fno-exceptions -fno-rtti -std=c++17
# 링크 옵션			
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static


# 규칙
# 대상: 필수항목
# 	레시피(명령열)
.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<