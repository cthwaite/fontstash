#pragma once
#include <vector>
#include <cstdlib>
namespace fontstash {
    // Atlas based on Skyline Bin Packer by Jukka Jylänki
    struct FONSatlasNode {
        short x, y, width;
    };

    struct FONSatlas {

        FONSatlas(int w, int h, int c) :
            width{w},
            height{h},
            nnodes_{0},
            cnodes_{c}
        {

            // Allocate space for skyline nodes_
            // nodes_ = (FONSatlasNode*)std::calloc(c, sizeof(FONSatlasNode));
            // if(nodes_ == nullptr) { throw std::bad_alloc(); }
            nodes_.resize(cnodes_);

            // Init root node.
            nodes_[0].x = 0;
            nodes_[0].y = 0;
            nodes_[0].width = (short)w;
            nnodes_++;
        }

        ~FONSatlas()
        {
            // if(nodes_ != NULL) { free(nodes_); }
        }

        int insertNode(int idx, int x, int y, int w)
        {
            // Insert node
            if (nnodes_+1 > cnodes_)
            {
                cnodes_ = cnodes_ == 0 ? 8 : cnodes_ * 2;
                // nodes_ = (FONSatlasNode*)std::realloc(nodes_, sizeof(FONSatlasNode) * cnodes_);
                // if(nodes_ == nullptr) throw
                nodes_.resize(cnodes_);
            }

            for(int i = nnodes_; i > idx; i--)
            {
                nodes_[i] = nodes_[i-1];
            }

            nodes_[idx].x = (short)x;
            nodes_[idx].y = (short)y;
            nodes_[idx].width = (short)w;
            nnodes_++;

            return 1;
        }

        void removeNode(int idx)
        {
            int i;
            if (nnodes_ == 0) return;
            for (i = idx; i < nnodes_-1; i++)
                nodes_[i] = nodes_[i+1];
            nnodes_--;
        }

        void expand(int w, int h)
        {
            // Insert node for empty space
            if (w > width)
            {
                insertNode(nnodes_, width, 0, w - width);
            }
            width = w;
            height = h;
        }

        void reset(int w, int h)
        {
            width = w;
            height = h;
            nnodes_ = 0;

            // Init root node.
            nodes_[0].x = 0;
            nodes_[0].y = 0;
            nodes_[0].width = (short)w;
            nnodes_++;
        }

        int addSkylineLevel(int idx, int x, int y, int w, int h)
        {
            int i;

            // Insert new node
            if (insertNode(idx, x, y+h, w) == 0)
                return 0;

            // Delete skyline segments that fall under the shadow of the new segment.
            for (i = idx+1; i < nnodes_; i++) {
                if (nodes_[i].x < nodes_[i-1].x + nodes_[i-1].width) {
                    int shrink = nodes_[i-1].x + nodes_[i-1].width - nodes_[i].x;
                    nodes_[i].x += (short)shrink;
                    nodes_[i].width -= (short)shrink;
                    if (nodes_[i].width <= 0) {
                        removeNode(i);
                        i--;
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }

            // Merge same height skyline segments that are next to each other.
            for (i = 0; i < nnodes_-1; i++) {
                if (nodes_[i].y == nodes_[i+1].y) {
                    nodes_[i].width += nodes_[i+1].width;
                    removeNode(i+1);
                    i--;
                }
            }

            return 1;
        }

        int rectFits(int i, int w, int h)
        {
            // Checks if there is enough space at the location of skyline span 'i',
            // and return the max height of all skyline spans under that at that location,
            // (think tetris block being dropped at that position). Or -1 if no space found.
            int x = nodes_[i].x;
            int y = nodes_[i].y;
            int spaceLeft;
            if (x + w > width)
                return -1;
            spaceLeft = w;
            while (spaceLeft > 0)
            {
                if (i == nnodes_) return -1;
                y = maxi(y, nodes_[i].y);
                if (y + h > height) return -1;
                spaceLeft -= nodes_[i].width;
                ++i;
            }
            return y;
        }

        int addRect(int rw, int rh, int* rx, int* ry)
        {
            int besth = height, bestw = width, besti = -1;
            int bestx = -1, besty = -1, i;

            // Bottom left fit heuristic.
            for (i = 0; i < nnodes_; i++) {
                int y = rectFits(i, rw, rh);
                if (y != -1) {
                    if (y + rh < besth || (y + rh == besth && nodes_[i].width < bestw)) {
                        besti = i;
                        bestw = nodes_[i].width;
                        besth = y + rh;
                        bestx = nodes_[i].x;
                        besty = y;
                    }
                }
            }

            if (besti == -1)
                return 0;

            // Perform the actual packing.
            if (addSkylineLevel(besti, bestx, besty, rw, rh) == 0)
                return 0;

            *rx = bestx;
            *ry = besty;

            return 1;
        }

        int nnodes() const { return nnodes_; }
        int cnodes() const { return cnodes_; }

        int width, height;
        // FONSatlasNode* nodes_;
        std::vector<FONSatlasNode> nodes_;
        int nnodes_;
        int cnodes_;
    };
}
