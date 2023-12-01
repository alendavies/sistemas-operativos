#define FUSE_USE_VERSION 30
#define _DEFAULT_SOURCE

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "types.h"

#define ROOT "/"

struct filesystem filesystem = {};

char fisopfs_file[MAX_PATH] = "file.fisopfs";

// Parsea el path recibido por parámetro
// Retorna NULL en caso de error
// Si no hay error retorna el nombre del archivo o directorio
static char *
parse_path(const char *path)
{
	if (strcmp(path, "/") == 0) {
		return (char *) path;
	}

	char *name = strdup(path + 1);

	const char *last_slash = strrchr(path, '/');
	if (last_slash == NULL) {
		return name;
	}

	char *f_name = strdup(last_slash + 1);

	free(name);

	return f_name;
}

// Devuelve un puntero al primer bloque disponible encontrado.
// Devuelve NULL en caso de no encontrarlo.
static block_t *
get_free_block()
{
	int i = 0;

	while (i < MAX_BLOCKS && filesystem.blocks[i].free_status != FREE) {
		i++;
	}

	if (i == MAX_BLOCKS) {
		printf("No se encontró un bloque vacío.\n");
		errno = ENOSPC;

		return NULL;
	}

	return &filesystem.blocks[i];
}

// Devuelve un puntero al bloque con la ruta correspondiente,
// o NULL de no encontrarlo.
static block_t *
get_block(const char *path)
{
	char *name = parse_path(path);

	int i = 0;

	while (i < MAX_BLOCKS && (strcmp(name, filesystem.blocks[i].path) != 0)) {
		i++;
	}

	if (i == MAX_BLOCKS) {
		printf("[debug] Error get block\n");

		return NULL;
	}

	return &filesystem.blocks[i];
}

void
get_parent_path(char *parent_path)
{
	char *last_slash = strrchr(parent_path, '/');

	if (last_slash != NULL) {
		*last_slash = '\0';
	} else {
		parent_path[0] = '\0';
	}
}

// Crea un nuevo bloque.
// Devuelve el índice del bloque, o -1 en caso de error.
int
new_block(const char *path, mode_t mode, int type)
{
	if (strlen(path) - 1 > MAX_CONTENT) {
		fprintf(stderr, "[debug] Error new_inode: %s\n", strerror(errno));
		errno = ENAMETOOLONG;
		return -ENAMETOOLONG;
	}

	char *name = parse_path(path);

	if (!name) {
		return -1;
	}

	block_t *new_block = get_free_block();

	if (!new_block) {
		printf("No hay espacio disponible.\n");
		errno = ENOSPC;

		return -1;
	}

	new_block->type = type;
	new_block->free_status = USED;
	new_block->mode = mode;
	new_block->size = 0;
	new_block->user_id = getuid();
	new_block->group_id = getgid();
	new_block->last_accessed_at = time(NULL);
	new_block->last_modified_at = time(NULL);

	strcpy(new_block->path, name);

	if (type == ARCH) {
		char parent_path[MAX_PATH];
		memcpy(parent_path, path + 1, strlen(path) - 1);
		parent_path[strlen(path) - 1] = '\0';

		get_parent_path(parent_path);

		if (strlen(parent_path) == 0) {
			strcpy(parent_path, ROOT);
		}

		strcpy(new_block->dir_path, parent_path);

	} else {
		strcpy(new_block->dir_path, ROOT);
	}

	memset(new_block->content, 0, sizeof(new_block->content));
	free(name);

	return 0;
}

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);

	return new_block(path, mode, DIR);
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create - path: %s\n", path);

	return new_block(path, mode, ARCH);
}

static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Getattr: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	st->st_dev = 0;
	st->st_uid = block->user_id;
	st->st_mode = block->mode;
	st->st_atime = block->last_accessed_at;
	st->st_mtime = block->last_modified_at;
	st->st_ctime = block->created_at;
	st->st_size = block->size;
	st->st_gid = block->group_id;
	st->st_nlink = 2;
	st->st_mode = __S_IFDIR | 0755;

	if (block->type == ARCH) {
		st->st_mode = __S_IFREG | 0644;
		st->st_nlink = 1;
	}

	return 0;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir(%s)\n", path);

	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Error readdir: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	if (block->type != DIR) {
		fprintf(stderr, "[debug] Error readdir: %s\n", strerror(errno));
		errno = ENOTDIR;
		return -ENOTDIR;
	}
	block->last_accessed_at = time(NULL);

	for (int i = 1; i < MAX_BLOCKS; i++) {
		if (filesystem.blocks[i].free_status == USED) {
			if (strcmp(filesystem.blocks[i].dir_path, block->path) ==
			    0) {
				filler(buffer, filesystem.blocks[i].path, NULL, 0);
			}
		}
	}

	return 0;
}

block_t
init_root()
{
	block_t root = {
		.free_status = USED,
		.type = DIR,
		.mode = __S_IFDIR | 0755,
		.size = MAX_DIRECTORY_SIZE,
		.user_id = 1717,
		.group_id = getgid(),
		.last_accessed_at = time(NULL),
		.last_modified_at = time(NULL),
		.created_at = time(NULL),
	};

	strcpy(root.path, ROOT);
	memset(root.content, 0, sizeof(root.content));
	strcpy(root.dir_path, "");

	return root;
}

block_t
init_block()
{
	block_t block = { 0 };

	return block;
}

int
init_fs()
{
	block_t root = init_root();

	filesystem.blocks[0] = root;

	// Init all blocks.
	for (int i = 1; i < MAX_BLOCKS; i++) {
		filesystem.blocks[i] = init_block();
	};

	return 0;
}

void *
fisopfs_init(struct fuse_conn_info *conn)
{
	printf("[debug] Initializing filesystem.\n");

	FILE *file = fopen(fisopfs_file, "r");

	if (!file) {
		init_fs();
	} else {
		int n = fread(&filesystem, sizeof(filesystem), 1, file);

		if (n != 1) {
			fprintf(stderr,
			        "[debug] Error init: %s\n",
			        strerror(errno));
			return NULL;
		}

		fclose(file);
	}

	return 0;
}

void
fisopfs_destroy(void *private_data)
{
	printf("[debug] fisop_destroy\n");

	FILE *file = fopen(fisopfs_file, "w");
	if (!file) {
		fprintf(stderr,
		        "[debug] Error saving filesystem: %s\n",
		        strerror(errno));
	}

	int n = fwrite(&filesystem, sizeof(filesystem), 1, file);
	if (n != 1) {
		fprintf(stderr,
		        "[debug] Error saving filesystem: %s\n",
		        strerror(errno));
	}

	fflush(file);
	fclose(file);
}

static int
fisopfs_flush(const char *path, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_flush(%s)\n", path);
	fisopfs_destroy(NULL);
	return 0;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s\n", path);

	if (size > MAX_CONTENT) {
		fprintf(stderr, "[debug] Error truncate: %s\n", strerror(errno));
		errno = EINVAL;
		return -EINVAL;
	}

	block_t *block = get_block(path);
	if (block == NULL) {
		fprintf(stderr, "[debug] Error truncate: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	block->size = size;
	block->last_modified_at = time(NULL);
	return 0;
}

static int
fisopfs_utimens(const char *path, const struct timespec tv[2])
{
	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Error utimens: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	block->last_accessed_at = tv[0].tv_sec;
	block->last_modified_at = tv[1].tv_sec;

	return 0;
}

static int
fisopfs_write(const char *path,
              const char *data,
              size_t size_data,
              off_t offset,
              struct fuse_file_info *fuse_info)
{
	printf("[debug] fisops_write - path: %s \n", path);

	if ((offset + size_data) > MAX_CONTENT) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(errno));
		errno = EFBIG;
		return -EFBIG;
	}

	block_t *block = get_block(path);

	if (block == NULL) {  // no existe --> lo creo
		int err = fisopfs_create(path, 33204, fuse_info);
		if (err < 0) {
			return err;
		}
		block = get_block(path);
		if (block == NULL) {
			fprintf(stderr,
			        "[debug] Error write: %s\n",
			        strerror(errno));
			errno = ENOENT;
			return -ENOENT;
		}
	}

	if (block->type != ARCH) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(errno));
		errno = EACCES;
		return -EACCES;
	}

	if (block->size < offset) {
		fprintf(stderr, "[debug] Error write: %s\n", strerror(errno));
		errno = EINVAL;
		return -EINVAL;
	}

	strncpy(block->content + offset, data, size_data);

	block->last_accessed_at = time(NULL);
	block->last_modified_at = time(NULL);
	block->size = strlen(block->content);
	block->content[block->size] = '\0';

	return (int) size_data;
}

static int
fisopfs_unlink(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);

	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Error unlink: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	if (block->type == DIR) {
		fprintf(stderr, "[debug] Error unlink: %s\n", strerror(errno));
		errno = EISDIR;
		return -EISDIR;
	}

	block->free_status = FREE;
	memset(block, 0, sizeof(block_t));

	return 0;
}

block_t **
search_files(const char *dir_path, int *n_files)
{
	int tope = 0;
	block_t **files = malloc(MAX_BLOCKS * sizeof(struct block *));

	char *path = parse_path(dir_path);
	if (!path) {
		return NULL;
	}

	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (strcmp(filesystem.blocks[i].dir_path, path) == 0) {
			files[tope++] = &filesystem.blocks[i];
		}
	}
	free(path);

	*n_files = tope;
	return files;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);

	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Error unlink: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	if (block->type != DIR) {
		fprintf(stderr, "[debug] Error rmdir: %s\n", strerror(errno));
		errno = ENOTDIR;
		return -ENOTDIR;
	}

	// chequear si tiene archivos adentro
	int n_files = 0;
	block_t **files = search_files(path, &n_files);
	if (!files) {
		fprintf(stderr,
		        "[debug] Error rmdir when allocating memory for path: "
		        "%s\n",
		        strerror(errno));
		errno = ENOMEM;
		return -ENOMEM;
	}
	free(files);

	if (n_files > 0) {
		fprintf(stderr, "[debug] Error rmdir: %s\n", strerror(errno));
		errno = ENOTEMPTY;
		return -ENOTEMPTY;
	}

	block->free_status = FREE;
	memset(block, 0, sizeof(block_t));

	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);

	if (offset < 0 || size < 0) {
		fprintf(stderr, "[debug] Error read: %s\n", strerror(errno));
		errno = EINVAL;
		return -EINVAL;
	}

	block_t *block = get_block(path);

	if (block == NULL) {
		fprintf(stderr, "[debug] Error read: %s\n", strerror(errno));
		errno = ENOENT;
		return -ENOENT;
	}

	if (block->type != ARCH) {
		fprintf(stderr, "[debug] Error read: %s\n", strerror(errno));
		errno = EISDIR;
		return -EISDIR;
	}

	char *file_content = block->content;
	size_t file_size = block->size;

	if (offset > file_size) {
		fprintf(stderr, "[debug] Error read: %s\n", strerror(errno));
		errno = EINVAL;
		return -EINVAL;
	}

	if (file_size - offset < size) {
		size = file_size - offset;
	}

	strncpy(buffer, file_content + offset, size);

	block->last_accessed_at = time(NULL);

	return size;
}

static struct fuse_operations operations = { .getattr = fisopfs_getattr,
	                                     .readdir = fisopfs_readdir,
	                                     .read = fisopfs_read,
	                                     .mkdir = fisopfs_mkdir,
	                                     .init = fisopfs_init,
	                                     .read = fisopfs_read,
	                                     .write = fisopfs_write,
	                                     .create = fisopfs_create,
	                                     .rmdir = fisopfs_rmdir,
	                                     .unlink = fisopfs_unlink,
	                                     .flush = fisopfs_flush,
	                                     .destroy = fisopfs_destroy,
	                                     .truncate = fisopfs_truncate,
	                                     .utimens = fisopfs_utimens };

int
main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &operations, NULL);
}
