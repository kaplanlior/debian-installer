/*
 * dpkg status file reading routines
 */

#include "main-menu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>
#include <locale.h>

/*
 * Read status file and generate and return a linked list of packages.
 */
struct package_t *status_read(void) {
	FILE *f;
	char *b, buf[BUFSIZE], *lang_code;
	int i;
	struct package_t *found, *newp, *p = 0;
	struct language_description *langdesc;

	tree_clear();

	if ((f = fopen(STATUSFILE, "r")) == NULL) {
		perror(STATUSFILE);
		return 0;
	}
	
	while (fgets(buf, BUFSIZE, f) && !feof(f)) {
		buf[strlen(buf)-1] = 0;
		if (strstr(buf, "Package: ") == buf) {
			newp = tree_add(buf + 9);
			newp->next = p;
			p = newp;
		}
		else if (strstr(buf, "installer-menu-item: ") == buf) {
			p->installer_menu_item = atoi(buf + 21);
		}
		else if (strstr(buf, "Status: ") == buf) {
			if (strstr(buf, " unpacked")) {
				p->status = unpacked;
			}
			else if (strstr(buf, " half-configured")) {
				p->status = half_configured;
			}
			else if (strstr(buf, " installed")) {
				p->status = installed;
			}
			else {
				p->status = other;
			}
		}
		else if (strstr(buf, "Description: ") == buf) {
			/*
			 * If there is already a description, it must be
			 * the translated one, which we prefer to use if
			 * possible.
			 */
			if (! p->description)
				p->description = strdup(buf+13);
		}
		else if (strstr(buf, "Description-") == buf &&
			 strlen(buf) >= 16 && buf[14] == ':') {
			lang_code = (char *) malloc(3);
			memcpy(lang_code, buf + 12, 2);
			lang_code[2] = 0;
			langdesc = malloc(sizeof (struct language_description));
			memset(langdesc,0,sizeof(struct language_description));
			langdesc->language = lang_code;
			langdesc->description = strdup(buf+16);
			if (p->localized_descriptions) 
				langdesc->next = p->localized_descriptions;
			p->localized_descriptions = langdesc;
		}
		else if (strstr(buf, "Depends: ") == buf) {
			/*
			 * Basic depends line parser. Can ignore versioning
			 * info since the depends are already satisfied.
			 */
			b=strdup(buf+9);
			i = 0;
			while (*b != 0 && *b != '\n') {
				if (*b != ' ') {
					if (*b == ',') {
						*b = 0;
						p->depends[++i] = 0;
					}
					else if (p->depends[i] == 0) {
						p->depends[i] = b;
					}
				}
				else {
					*b = 0; /* eat the space... */
				}
				b++;
			}
			*b = 0;
			p->depends[i+1] = 0;
		}
		else if (strstr(buf, "Provides: ") == buf) {
			/*
			 * A provides causes a fake package to be made,
			 * that depends on the package that provides it. If
			 * the fake package already exists, just add the
			 * providing package to its dependancy list. This
			 * means that virtual packages are actually ANDed
			 * for the purposes of this program.
			 */
			if ((found = tree_find(buf + 10))) {
				newp=found;
			}
			else {
				newp = tree_add(buf + 10);
				newp->next=p->next;
				p->next=newp;
			}
			for (i=0; newp->depends[i] != 0; i++);
			newp->depends[i] = p->package;
			newp->depends[i+1] = 0;
		}
	}
	fclose(f);
	
	return p;
}
