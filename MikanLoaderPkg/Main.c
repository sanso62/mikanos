#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/PrintLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include  <Protocol/DiskIo2.h>
#include  <Protocol/BlockIo.h>

struct MemoryMap{
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

// 메모리 맵 취득
EFI_STATUS GetMemoryMap(struct MemoryMap* map){
  // 메모리 영역이 작아 메모리 맵을 전부 담기가 불가능한 경우
  if (map->buffer == NULL){
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;

  // gBS: 메모리 관리관련 기능이 포함된 부트 서비스 글로벌 변수
  // 부트 서비스:  UEFI가 OS를 기동하기 위해 필요한 기능을 제공
  return gBS->GetMemoryMap(
    &map->map_size,                       // 메모리 맵 크기
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,  // 메모리 맵 시작 포인터
    &map->map_key,                        // 메모리 맵 식별자
    &map->descriptor_size,                // 메모리 맵 각 행을 나타내는 메모리 디스크립터 크기
    &map->descriptor_version              // 메모리 디스크립터 버전
  );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

// 메모리 맵 정보를 csv파일에 기록
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file){
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header = 
      "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
      map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter; // iter: 메모리 디스크립터 어드레스
  int i; // i: 메모리 뱁의 행 번호를 나타내는 카운터
  for(iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i=0;
      iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
      iter += map->descriptor_size, i++){
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

    // AsciiPrint()는 EDK II에서 준비한 라이브러리 함수로, 지정한 char 배열에 가공한 문자열을 기록
    len = AsciiSPrint(
      buf, sizeof(buf),
      "%u, %x, %-ls, %08lx, %lx, %lx\n",
      i, desc->Type, GetMemoryTypeUnicode(desc->Type),
      desc->PhysicalStart, desc->NumberOfPages,
      desc->Attribute & 0xffffflu);

    // 파일에 문자열을 씀
    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(
      image_handle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&loaded_image,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(
      loaded_image->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&fs,
      image_handle,
      NULL,
      EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, Mikan World!\n");
  
  // 메모리 맵으로 사용할 변수 선언. 사이즈 16KiB
  CHAR8 memmap_buf[4096 * 4];
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};

  // 메모리 맵 취득
  GetMemoryMap(&memmap);

  // 메모리 맵을 저장할 파일 쓰기 모드로 열기
  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);
  EFI_FILE_PROTOCOL* memmap_file;
  root_dir->Open(
    root_dir, &memmap_file, L"\\memmap",
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0
  );

  // 메모리 맵을 파일에 저장
  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  Print(L"All done\n");

  while(1);
  return EFI_SUCCESS;
}
