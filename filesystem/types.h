#include <stdbool.h>
#include <time.h>

#define MAX_PATH 100      // Largo máximo de un path.
#define MAX_CONTENT 1024  // Máximo contenido de un archivo.
#define MAX_DIRECTORY_SIZE 1024
#define MAX_BLOCKS 100  // Máxima cantidad de blocks.
#define FREE 0
#define USED 1

typedef enum file_type {
	ARCH,  // archivo
	DIR    // directorio
} file_type_t;

typedef struct block {
	int free_status;  // FREE o USED
	file_type_t type;
	mode_t mode;
	size_t size;
	uid_t user_id;
	gid_t group_id;
	time_t last_accessed_at;
	time_t last_modified_at;
	time_t created_at;
	char path[MAX_PATH];
	char content[MAX_CONTENT];
	char dir_path[MAX_PATH];
} block_t;

struct filesystem {
	struct block blocks[MAX_BLOCKS];
};
