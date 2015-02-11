#include "hlbvh.h"

#include <algorithm>
#include <stack>
#include <cassert>
#include <iostream>

// The following two functions are from
// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
static unsigned int ExpandBits(unsigned int v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
unsigned int CalcMortonCode(float3 p)
{
    float x = std::min(std::max(p.x * 1024.0f, 0.0f), 1023.0f);
    float y = std::min(std::max(p.y * 1024.0f, 0.0f), 1023.0f);
    float z = std::min(std::max(p.z * 1024.0f, 0.0f), 1023.0f);
    unsigned int xx = ExpandBits((unsigned int)x);
    unsigned int yy = ExpandBits((unsigned int)y);
    unsigned int zz = ExpandBits((unsigned int)z);
    return xx * 4 + yy * 2 + zz;
}

// Count the number of leading zeroes in x
static int nlz(unsigned int x)
{
    if (x == 0) return 32;
    
    int n = 0;
    if (x <= 0x0000FFFF) { n=n+16; x=x<<16; }
    if (x <= 0x00FFFFFF) { n=n+8; x=x<<8; }
    if (x <= 0x0FFFFFFF) { n=n+4; x=x<<4; }
    if (x <= 0x3FFFFFFF) { n=n+2; x=x<<2; }
    if (x <= 0x7FFFFFFF) { n=n+1;}
    return n;
}


void Hlbvh::BuildImpl(std::vector<Primitive*> const& prims)
{
    // STEP 1: assign 30-bit Morton codes to each primitive
    // Morton code is calculated based on bounding box center
    std::vector<std::pair<int, unsigned int> > mortoncodes(prims.size());
    
#pragma omp parallel
    for (int i=0;i<(int)prims.size(); ++i)
    {
        mortoncodes[i] = std::make_pair(i, CalcMortonCode(prims[i]->Bounds().center()));
    }
    
    // STEP 2: sort primitives by their Morton codes
    std::sort(mortoncodes.begin(), mortoncodes.end(),
    [](std::pair<int, unsigned int> const& p1, std::pair<int, unsigned int> const& p2)
    {
        return p1.second < p2.second;
    });
    
    // STEP 3: start finding splits and generating hierarchy
    // Structure describing split request
    struct SplitRequest
    {
        int left;
        int right;
        Node** ptr;
    };
    
    // Prepare the stack
    std::stack<SplitRequest> stack;
    
    // Initial request is processing the whole thing
    SplitRequest initialrequest = {0, (int)mortoncodes.size()-1, nullptr};
    
    stack.push(initialrequest);
    
    // Start popping requests
    while (!stack.empty())
    {
        // Fetch new request
        SplitRequest req = stack.top();
        stack.pop();
        
        // Prepare new node
        nodes_.push_back(std::unique_ptr<Node>(new Node));
        Node* node = nodes_.back().get();
        node->bounds = bbox();
        
        // Calc bbox
        for (int i = req.left; i <= req.right; ++i)
        {
            node->bounds = bboxunion(node->bounds, prims[mortoncodes[i].first]->Bounds());
        }
        
        // Create leaf node if there is only 1 primitive
        if (req.left == req.right)
        {
            node->type = kLeaf;
            node->startidx = (int)primitives_.size();
            node->numprims = 1;
            primitives_.push_back(prims[mortoncodes[req.left].first]);
        }
        else
        {
            node->type = kInternal;
            
            // Find split based on Morton codes
            int split = FindSplit(mortoncodes, req.left, req.right);
            
            // Check the split is correct
            assert(split >= req.left && split <= req.right);
            
            //std::cout <<"Split " << split << "\n";
            
            // Left request
            SplitRequest leftrequest = {req.left, split, &node->lc};
            // Right request
            SplitRequest rightrequest = {split + 1, req.right,  &node->rc};
            
            // Put those to stack
            stack.push(leftrequest);
            stack.push(rightrequest);
        }
        
        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }
    
    // Set root_ pointer
    root_ = nodes_[0].get();
}

int Hlbvh::FindSplit(std::vector<std::pair<int,unsigned int> > const& mortoncodes, int left, int right)
{
    // Fetch codes for both ends
    unsigned int leftcode = mortoncodes[left].second;
    unsigned int rightcode = mortoncodes[right].second;
    
    // Split in halves if they are identical
    if (leftcode == rightcode)
        return (left + right) >> 1;
    
    // Calculate the number of identical bits from higher end
    int numidentical = nlz(leftcode ^ rightcode);
    
    do
    {
        // Proposed split
        int newsplit = (right + left) / 2;
        
        // Fetch code for the split
        unsigned int splitcode = mortoncodes[newsplit].second;
            
        // If it has more equal leading bits than left and right accept it
        if (nlz(leftcode ^ splitcode) > numidentical)
        {
            left = newsplit;
        }
        else
        {
            right = newsplit;
        }
    }
    while (right > left + 1);
    
    return left;
}