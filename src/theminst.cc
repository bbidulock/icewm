#define _XOPEN_SOURCE 700
#include "config.h"
#include "theminst.h"
#include "base.h"
#include "yapp.h"
#include "ytime.h"
#include <ftw.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define ICEWM_EXTRA_TAR \
"https://github.com/ice-wm/icewm-extra/releases/download/themes/icewm-extra.tar.lz"
static const char extra_url[] = ICEWM_EXTRA_TAR;

static int treenode(const char* path, const struct stat* sb,
                    int typeflag, struct FTW* ftwbuf) {
    return remove(path);
}

int remove_directory_tree(const char* path) {
    return nftw(path, treenode, 4, FTW_DEPTH | FTW_PHYS);
}

static char* get_tar() {
#if __OpenBSD__ || __NetBSD__ || __FreeBSD__
    char* gtar = path_lookup("gtar");
    if (gtar) return gtar;
#endif
    return path_lookup("tar");
}

static void install_extra(const char* name, bool* result) {
    if (isEmpty(name) || strpbrk(name, "/&();*<>`{}|!?")) {
        errno = EINVAL;
        return fail("%s", name);
    }
    upath dir(YApplication::getPrivConfDir(true));
    if ( !dir.dirExists()) {
        errno = ENOENT;
        return fail("%s", dir.string());
    }
    dir += "themes";
    if ( !dir.dirExists() && dir.mkdir()) {
        return fail("%s", dir.string());
    }
    if ( strcmp(name, "list") && (dir + name).dirExists()) {
        errno = EEXIST;
        return fail("%s", (dir + name).string());
    }

    if (dir.isWritable() == false) {
        return fail("%s", dir.string());
    }
    if (dir.chdir()) {
        return fail("%s", dir.string());
    }

    const char* base = my_basename(extra_url);
    struct stat node;
    if (stat(base, &node) == 0) {
        long now = seconds();
        long lim = 31 * 86400L;
        if (now - node.st_ctime > lim ||
            node.st_size < 1024*1024) {
            printf("Removing stale %s\n", base);
            remove(base);
        }
        else if (access(base, R_OK) && chmod(base, 0444)) {
            remove(base);
        }
    }

    csmart tar(get_tar());
    if ( !tar) {
        errno = ENOENT;
        return fail("tar");
    }
    csmart zip(path_lookup("lzip"));
    if ( !zip) {
        errno = ENOENT;
        return fail("lzip");
    }

    if (access(base, F_OK)) {
        csmart wget(path_lookup("wget"));
        csmart curl(path_lookup("curl"));
        if ( !wget && !curl) {
            errno = ENOENT;
            return fail("wget or curl");
        }
        printf("Downloading %s with %s...\n",
                base, wget ? "wget" : "curl");
        fflush(stdout);

        int pid = fork(), status = 0;
        if (pid == -1) {
            return perror("fork");
        }
        if (pid == 0) {
            if (wget) {
                if (execlp(wget, "wget",
                           "--dns-timeout=5",
                           "--connect-timeout=5",
                           "--max-redirect=1",
                           "-O", base,
                           "--unlink",
                           "--no-verbose",
                           extra_url, nullptr)) {
                    perror(wget);
                }
            }
            else if (curl) {
                if (execlp(curl, "curl",
                           "--connect-timeout", "5",
                           "--fail", "--globoff",
                           "--keepalive-time", "2",
                           "--location",
                           "--max-redirs", "1",
                           "--output", base,
                           "--remove-on-error",
                           "--silent", "--show-error",
                           "--url", extra_url, nullptr)) {
                    perror(curl);
                }
            }
            _exit(1);
        }
        else if (waitpid(pid, &status, 0) == -1) {
            return fail("waitpid");
        }
        else if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                errno = 0;
                return msg("download of icewm-extra failed");
            }
        }
        else if (WIFSIGNALED(status)) {
            errno = 0;
            return msg("download of icewm-extra failed");
        }
        else {
            printf("Download completed\n");
        }
    }
    if (access(base, R_OK) || stat(base, &node)) {
        return fail("%s", base);
    }
    if (node.st_size < 1024*1024) {
        remove(base);
        errno = EINVAL;
        return fail("%s", base);
    }

    char temp[] = "temporary-XXXXXX";
    if (strcmp(name, "list") == 0) {
        char buf[1234];
        snprintf(buf, sizeof buf, "%s -cd %s | %s -tf -",
                 (char *) zip, base, (char *) tar);
        FILE* fp = popen(buf, "r");
        if (fp) {
            int k = 0;
            char line[1234];
            while (fgets(line, sizeof line, fp)) {
                char* a = strchr(line, '/');
                if (a) {
                    char* b = strchr(++a, '/');
                    if (b && b[1] == '\n') {
                        if (++k > 1)
                            putchar(' ');
                        fwrite(a, 1, b - a, stdout);
                    }
                }
            }
            if (k)
                putchar('\n');
            pclose(fp);
        }
        *result = true;
    }
    else if (mkdtemp(temp) == nullptr) {
        return fail("mkdtemp");
    }
    else {
        upath want(upath(temp) + "icewm-extra" + name);
        char buf[1234];
        snprintf(buf, sizeof buf,
                 "%s -cd %s | %s -xf - -C %s icewm-extra/%s",
                 (char *) zip, base, (char *) tar, temp, name);
        int res = system(buf);
        if (res == -1) {
            return fail("system(\"%s\")", buf);
        }
        else if (res || !want.dirExists()) {
            warn("Failed to extract theme %s from archive.", name);
        }
        else if (rename(want.string(), name)) {
            warn("Failed to rename %s to %s.", want.string(), name);
        }
        else {
            *result = true;
            printf("Theme %s installed successfully.\n", name);
        }
        remove_directory_tree(temp);
    }
}

void install_theme(const char* name) {
    bool result = false;
    install_extra(name, &result);
    exit(result ? 0 : 1);
}

