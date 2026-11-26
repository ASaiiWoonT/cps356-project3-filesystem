# CPS 356 Project 3 — Disk and persistence reference

This annotated C11 reference demonstrates the virtual disk, Sai’s part of the project. The project handout requires team-authored C for submission. Use this copy for learning and testing.

## Build and run

```sh
cmake -S . -B build
cmake --build build
demo_dir=$(mktemp -d)
./build/Project3 seed "$demo_dir/example.bin"
./build/Project3 show "$demo_dir/example.bin"
```

`seed` creates one image with the root, `/school`, and a recycled `notes.txt` holding five bytes. It requires a new image path. `show` runs as a second process, reads the saved image, and prints:

```text
ID 1, parent 0: /
ID 2, parent 1: school
ID 3, parent 2: notes.txt [recycled from /school] = hello
Free slots: 29
```

| File | Where to start |
| --- | --- |
| `disk.h` | Read the fields of `Item` and `Disk`. An item's ID identifies it, and its parent ID identifies the directory containing it. |
| `disk.c` | Follow `disk_init`, then `disk_allocate`, `disk_save`, and `disk_load`. `disk_release` makes a recycled slot available again. |
| `main.c` | Trace `seed` and `show`. The sample it prints is intentionally small enough to follow in a debugger. |

The starter checked `slot = -1` before searching for a free record. The reference scans for a `FREE` record in `disk_allocate` and returns a pointer or `NULL`. Recycled records keep their data and count as occupied. Permanent release clears the record, then a later allocation can use that slot with a fresh ID.

The image stores a version signature, next ID, and table of 32 records, including root. Each file can hold up to 512 bytes. The format uses the native C layout and requires a compatible build. Run one writer at a time. Saving writes a temporary file beside the image and replaces the old image only after the write and close succeed. This supports clean restarts and detects incomplete images. Power-loss durability would need additional synchronization.

## Team handoff

The demo is an entry point for the storage reference. The team shell will take over startup, dispatch, and exit when Member 2 adds it. The ownership and behavior below are proposed. **Due November 30, 2026 at 11:55 p.m. Eastern.**

## Ownership

| Member | Owns | Proposed files | First pull request |
| --- | --- | --- | --- |
| 1 — Sai | Virtual disk, record allocation, persistence, build and integration | `disk.c`, `disk.h`, `CMakeLists.txt` | Publish the shared disk model and agree on how the shell and bin access it |
| 2 | Shell loop, quoted input, path handling, active file and directory commands | `main.c`, `shell.c`, `fs.c`, `fs.h` | Quoted parsing, current directory, `mkdir`, `cd`, `pwd`, and `ls` |
| 3 | Recycling, listing the bin, restoring, permanent deletion, bin test cases | `recycle.c`, `recycle.h` | `rm` and `bin`, including deletion metadata and empty-directory checking |

### Member 1 — Disk and persistence

- [ ] Define the shared records and limits with the team. The starter uses 32 slots including the root. Agree on the name and content limits before the command modules depend on them.
- [ ] Initialize a new image in memory with one active root. Allocate a fresh item ID on creation or copy. Keep the next ID in the image so slot reuse after restart cannot give a new item an old identity.
- [ ] Expose allocation, lookup by ID, and permanent release. Count both active and recycled items toward capacity. Report when no slot is available.
- [ ] Save and load the root, active and recycled records, file bytes, deletion metadata, and next ID through one binary image. Distinguish a missing image from an unreadable or invalid one.
- [ ] Verify restart and disk-full behavior with the command owners. Maintain the build target and document the chosen disk image path for integration testing.

### Member 2 — Shell and active file operations

Work in this order so Member 3 can use the path and lookup behavior early.

- [ ] **Shell loop and dispatch.** Start in `/`, display the current path in the prompt, read one line at a time, and dispatch by command and argument count. Preserve spaces inside quotes, including an empty quoted string. Reject malformed input with a clear message and keep the shell usable. `exit` asks Member 1’s disk module to save, then terminates when the save succeeds.
- [ ] **Paths and lookups.** Resolve an absolute path from `/` and a relative path from the current directory. Support `.` and `..` and keep `..` at the root in `/`. Use the current directory’s ID so `pwd` can build its path even after an ancestor is renamed. Resolve active items only. Give Member 3 a way to find an active child by parent ID and name, find a destination directory, and determine whether a directory has any active children.
- [ ] **Directories.** `mkdir [dirname]` validates the name, rejects a duplicate active sibling, and allocates one directory record. `cd [path]` accepts an existing active directory and preserves the old location on failure. `pwd` prints its absolute path. `ls` lists active children in the current directory and shows file sizes and an empty-directory message.
- [ ] **File creation and contents.** `touch [filename]` creates a zero-length file. `write [filename] "text"` replaces the contents of an existing active file under the proposed team behavior. Validate the byte limit before changing the contents or size. `read [filename]` displays the stored bytes. Reject directory targets for `write` and `read`.
- [ ] **Copy and move.** `cp [src] [dest]` copies a file into a new record with a fresh ID and independent contents. Handle disk full before writing any new record. `mv [src] [dest]` can rename or move a file or directory. Preserve item identity and contents, reject root moves and name conflicts, and reject moving a directory into itself or a descendant. Under the proposed destination rule, an existing directory receives the item under its current name. Otherwise the destination’s last component is the new name, with an existing parent required.
- [ ] **Verify and document.** Demonstrate quoted names and text, absolute and relative navigation, an independent copy, a move of a directory with children, and a failed move that leaves the original tree intact. Capture actual outputs for the report and explain how parent IDs produce paths during the evaluation.

### Member 3 — Recycle Bin and recovery

Start with `rm` and `bin`. Once they agree about which records are visible, add restore and permanent deletion.

- [ ] **Soft deletion.** `rm [name]` finds an active child in the current directory. Accept a file or empty directory. Report missing names, reject the root and nonempty directories, and leave state unchanged on failure. Record the original filename, parent ID, original directory path, and deletion time. Preserve its kind, contents, size, and item ID. Mark it recycled so normal commands hide it while the slot remains occupied.
- [ ] **Inspect the bin.** `bin` lists recycled items across the whole disk, regardless of the current directory. Show the name, original directory path, and file size as in the handout’s sample. Display deletion time and whether the entry is a file or directory. Provide a clear empty-bin message.
- [ ] **Restore to the original location.** `restore [name]` finds the selected recycled record and looks up its original parent by ID. The parent must still be an active directory. A directory newly created at the same path has a different ID. Before changing anything, reject a missing or inactive parent or an active sibling with the same name. On success, make the existing record active and keep its bytes and item ID.
- [ ] **Restore to a chosen directory.** `restore [name] [destination]` uses Member 2’s path rules to locate an existing active directory. Apply the same collision check there. On success, update the parent ID and make the existing record active. On any failure, the original bin entry and destination stay intact.
- [ ] **Permanently reclaim slots.** `delete [name]` finds a recycled entry and calls Member 1’s release operation. An unknown name gets a bin-specific error even if an active file has that name. `emptybin` releases every recycled slot and preserves all active records. Verify that an erased slot accepts a new item even after the image is reloaded.
- [ ] **Handle duplicate bin names.** Two different directories can each contain `notes.txt`. Agree on a documented selection rule before building `restore` or `delete`. Proposed rule: both commands select the most recently recycled matching name, break equal-time ties by item ID, and use the same order in `bin`. Test two same-named entries and confirm the chosen entry’s original location and contents.
- [ ] **Verify and document.** Demonstrate hidden-but-occupied, original-location restore, alternate restore, conflicts, missing or recycled parents, directory recovery, and slot reclamation. Capture actual outputs for the report and explain the state transitions during the evaluation.

## Shared decisions before coding across branches

| Decision | Proposed agreement |
| --- | --- |
| Ownership of state | The disk module owns the record table and ID allocation. The shell passes the same disk and current directory ID to active and recycle commands. |
| Visibility | Active lookups and `ls` use active records. Bin commands select recycled records. Allocation uses free slots. |
| Name and path rules | Reject duplicate active names within a single directory. Use one shared path resolver for `cd`, destination paths, and alternate restore. Agree on name limits, case sensitivity, and whether existing `touch` is a no-op or an error. |
| Errors | Validate all required inputs before updating a record. Distinguish missing item, wrong type, name conflict, invalid path or arguments, and disk full where applicable. |
| Startup and save | Start at root. Load a valid image at launch, initialize only when the image is absent, report load/save failures, and keep the existing image intact on failed load. `exit` saves as required. Agree whether to also save after each successful change. |
| Integration | Member 2 owns command routing. Member 3 provides the bin handlers and uses the shared path helpers. Member 1 owns the image format and permanent slot release. Agree on the interfaces in the first pull request so both command branches can start together. |

## Integration checks

Start with a fresh test image for each scenario. The commands below are simulator inputs. Record actual terminal results for the report.

| Scenario | Input or setup | Expected result |
| --- | --- | --- |
| Quoted input and copy | Create and enter `"OS Project"`. Create `"Final Report.txt"`, write `"hello world"`, copy it, rewrite the original, and read the copy. | The prompt and `pwd` show `/OS Project`. The copy still contains `hello world` and `ls` shows byte counts. |
| Move a subtree | Make a directory with a child file, move or rename the directory, then find the child under the new path. Try moving the directory into one of its descendants. | The child follows the directory. The cycle attempt fails and the tree stays intact. |
| Recycle across restart | In `/school`, create `notes.txt`, write `hello`, recycle it, and `exit`. Reopen the same image, enter `/school`, run `ls` and `bin`, restore the file, then read it. Exit and reopen once more. | On the first reopen the file is in the bin and absent from `ls`. Restore returns `hello`. On the second reopen it’s active. |
| Missing original parent | Recycle `/school/notes.txt`, remove its now-empty parent, and try `restore notes.txt`. Then run `restore notes.txt /`. | Original restore fails. Alternate restore puts the original file and contents in `/`. |
| Conflicting name | Recycle a file, create a new file with the same name and different contents in its former directory, then try restoring the older one. | The new file retains its contents and the older one remains in the bin. |
| Directory rules | Try to remove a directory with an active child. Then recycle and restore a separate empty directory. | The first removal fails. The empty directory returns as a directory. |
| Full disk and permanent deletion | Fill all slots, `rm` one file, and try creating another. Permanently `delete` the recycled file and try creating again. | The first creation fails. One new item can be created after permanent deletion. |
| Empty bin and unknown entry | Recycle multiple entries, run `emptybin`, and try `restore` and `delete` with an absent name. Restart and run `bin` again. | Both missing-entry commands report useful errors. The bin stays empty after restart and active files are intact. |

## GitHub workflow

Use a private team repository with `main` as the integration branch. Once the shared interface is committed to `main`, open a pull request from `member-1/disk`, `member-2/shell-fs`, or `member-3/recycle-bin` for each member’s changes. Pull requests should include the commands tested, observed results, and any changed shared interface. Keep build outputs, editor settings, and local disk images out of commits.

Merge the shared disk and path interfaces first, then bring in the command modules. Run the integration checks against the merged version on a fresh image and again across restarts. Each member should record the features they implemented, the tests they ran, and what they can explain during the evaluation.

## Canvas submission

Prepare one team ZIP named with all three student IDs. The handout asks for documented source in `Code/`, `Report.pdf` (up to 10 pages, with solution explanations, results and analysis, and individual contributions), `Presentation.pptx` (up to 15 slides), and `README.txt` with the final build and run instructions. Use the tested command, executable name, image location, and a short sample run in the submitted `README.txt`. This GitHub README is the team’s work plan.
