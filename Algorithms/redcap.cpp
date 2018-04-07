/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created 5/30/2017 lixun910@gmail.com
 */

#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <iterator>
#include <cstdlib>

#include <wx/textfile.h>
#include <wx/stopwatch.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>

#include "../ShapeOperations/GalWeight.h"
#include "../logger.h"
#include "../GenUtils.h"
#include "cluster.h"
#include "redcap.h"

using namespace std;
using namespace boost;

typedef map<pair<SpanningTree*, SpanningTree*>, double> SubTrees;

/////////////////////////////////////////////////////////////////////////
//
// RedCapNode
//
/////////////////////////////////////////////////////////////////////////
RedCapNode::RedCapNode(int _id, const vector<double>& _value)
: value(_value)
{
    id = _id;
}

RedCapNode::RedCapNode(RedCapNode* node)
: value(node->value)
{
    id = node->id;
    neighbors = node->neighbors;
}

RedCapNode::~RedCapNode()
{
}

void RedCapNode::AddNeighbor(RedCapNode* node)
{
    neighbors.insert(node);
}

void RedCapNode::RemoveNeighbor(RedCapNode* node)
{
    neighbors.erase(neighbors.find(node));
}

void RedCapNode::RemoveNeighbor(int node_id)
{
    for (it=neighbors.begin(); it!=neighbors.end(); it++) {
        if ( (*it)->id == node_id ) {
            neighbors.erase(it);
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////
//
// RedCapEdge
//
/////////////////////////////////////////////////////////////////////////
RedCapEdge::RedCapEdge(RedCapNode* _a, RedCapNode* _b, double _l, double _w)
{
    a = _a;
    b = _b;
    weight = _w;
    length = _l;
}

RedCapEdge::~RedCapEdge()
{
    // node will be released at it's container
}

/////////////////////////////////////////////////////////////////////////
//
// SpanningTree
//
/////////////////////////////////////////////////////////////////////////
SpanningTree::SpanningTree(const vector<RedCapNode*>& all_nodes,
                           const vector<vector<double> >& _data,
                           const vector<bool>& _undefs,
                           double* _controls, double _control_thres)
: data(_data), undefs(_undefs), controls(_controls), control_thres(_control_thres)
{
    left_child = NULL;
    right_child = NULL;
    root = NULL;
    ssd = -1;
    
    for (int i=0; i<all_nodes.size(); i++) {
        node_dict[all_nodes[i]->id] = false;
    }
}

SpanningTree::SpanningTree(RedCapNode* node,
                           RedCapNode* exclude_node,
                           const vector<vector<double> >& _data,
                           const vector<bool>& _undefs,
                           double* _controls, double _control_thres)
: data(_data), undefs(_undefs),controls(_controls), control_thres(_control_thres)
{
    left_child = NULL;
    right_child = NULL;
    root = NULL;
    ssd = -1;
    
    // construct spanning tree from root
    unordered_map<int, bool> visited;
    list<RedCapNode*> stack;
    stack.push_back(node);
    while (!stack.empty()) {
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        node_dict[tmp->id] = false;
        
        RedCapNode* new_node = getNewNode(tmp);
        if (root==NULL) root = new_node;
        
        set<RedCapNode*>& nbrs = tmp->neighbors;
        set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id == exclude_node->id)
                continue;
            
            if (visited.find(nn->id) == visited.end()) {
                stack.push_back(nn);
            }
            visited[nn->id] = true;
            
            // local copy node,
            RedCapNode* new_nn = getNewNode(nn);
            // local copy edges
            addNewEdge(new_node, new_nn);
            // add neighbor info
            new_node->AddNeighbor(new_nn);
        }
    }
}

SpanningTree::~SpanningTree()
{
    if (left_child)
        delete left_child;
    if (right_child)
        delete right_child;
    for (int i=0; i<new_edges.size(); i++) {
        delete new_edges[i];
    }
    unordered_map<int, RedCapNode*>::iterator it;
    for (it=new_nodes.begin(); it != new_nodes.end(); it++) {
        delete it->second;
    }
}

bool SpanningTree::checkEdge(RedCapEdge *edge)
{
    RedCapNode* a = edge->a;
    RedCapNode* b = edge->b;
    
    if ( edge_dict.find(make_pair(a->id, b->id)) != edge_dict.end() ||
         edge_dict.find(make_pair(b->id, a->id)) != edge_dict.end() )
        return true;
    return false;
}

void SpanningTree::addNewEdge(RedCapNode* a, RedCapNode* b)
{
    if (edge_dict.find(make_pair(a->id, b->id)) == edge_dict.end() &&
        edge_dict.find(make_pair(b->id, a->id)) == edge_dict.end() )
    {
        RedCapEdge* e = new RedCapEdge(a, b, -1);
        edge_dict[ make_pair(a->id, b->id)] = true;
        edges.push_back(e);
        new_edges.push_back(e); // clean memeory
    }
}

RedCapNode* SpanningTree::getNewNode(RedCapNode* old_node, bool copy_nbrs)
{
    if (new_nodes.find(old_node->id) == new_nodes.end()) {
        RedCapNode* new_node = new RedCapNode(old_node->id, old_node->value);
        if (copy_nbrs) {
            std::set<RedCapNode*>::iterator it;
            std::set<RedCapNode*>::iterator begin = old_node->neighbors.begin();
            std::set<RedCapNode*>::iterator end = old_node->neighbors.end();
            for (it= begin; it!= end; it++) {
                RedCapNode* nbr_node = getNewNode(*it);
                new_node->AddNeighbor(nbr_node);
            }
        }
        new_nodes[ old_node->id ] = new_node;
        return new_node;
    }
    return new_nodes[ old_node->id ];
}

bool SpanningTree::AddEdge(RedCapEdge *e)
{
    if (!checkEdge(e)) {
        // add edge
        cout << e->a->id << "--" << e->b->id << endl;
        edge_dict[ make_pair(e->a->id, e->b->id) ] = true;
        
        RedCapNode* a = getNewNode(e->a);
        RedCapNode* b = getNewNode(e->b);

        node_dict[a->id] = true;
        node_dict[b->id] = true;
        
        if (root == NULL) {
            root = a;
        }
        
        // local nodes will reconstruct their neigbhors
        a->AddNeighbor(b);
        b->AddNeighbor(a);

        e->a = a; // replace with local node
        e->b = b;
        edges.push_back(e);
        
        return true;
    }
    return false;
}

bool SpanningTree::IsFullyCovered()
{
    // check if fully covered
    unordered_map<int, bool>::iterator _it;
    for (_it = node_dict.begin(); _it != node_dict.end(); _it++) {
        if (_it->second == false) {
            return false;
        }
    }
    
    // breath-first search
    unordered_map<int, bool> nd;
    list<RedCapNode*> stack;
    stack.push_back(root);
    std::set<RedCapNode*>::iterator it;
    
    int cnt = 0;
    while (!stack.empty()) {
        RedCapNode* node = stack.front();
        stack.pop_front();
        
        cout << node->id << endl;
        cnt ++;
        
        nd[node->id] = true;
        
        for (it = node->neighbors.begin(); it != node->neighbors.end(); it++) {
            if (nd.find((*it)->id) == nd.end()) {
                stack.push_back(*it);
                nd[(*it)->id] = true;
            }
        }
    }
    return cnt == data.size();
}

void SpanningTree::Split()
{
    int n_tasks = edges.size();

    int nCPUs = boost::thread::hardware_concurrency();
    int quotient = n_tasks / nCPUs;
    int remainder = n_tasks % nCPUs;
    int tot_threads = (quotient > 0) ? nCPUs : remainder;
    
    cand_trees.clear();
    boost::thread_group threadPool;
    for (int i=0; i<tot_threads; i++) {
        int a=0;
        int b=0;
        if (i < remainder) {
            a = i*(quotient+1);
            b = a+quotient;
        } else {
            a = remainder*(quotient+1) + (i-remainder)*quotient;
            b = a+quotient-1;
        }
        boost::thread* worker = new boost::thread(boost::bind(&SpanningTree::subSplit, this, a, b));
        threadPool.add_thread(worker);
    }
    threadPool.join_all();
    
    // merge results
    bool is_first = true;
    double best_hg;
    RedCapEdge* best_e;
    
    map<RedCapEdge*, double>::iterator it;
    for (it = cand_trees.begin(); it != cand_trees.end(); it++) {
        RedCapEdge* e = it->first;
        double hg = it->second;

        if (is_first || (best_hg >0 && best_hg < hg) ) {
            best_hg = hg;
            is_first = false;
            best_e = e;
        }
    }
    left_child = new SpanningTree(best_e->a, best_e->b, data, undefs, controls, control_thres);
    right_child = new SpanningTree(best_e->b, best_e->a, data, undefs, controls, control_thres);
}

bool SpanningTree::quickCheck(RedCapNode* node, RedCapNode* exclude_node)
{
    if (controls == NULL)
        return false;
    
    double check_val = 0;
    unordered_map<int, bool> visited;
    
    list<RedCapNode*> stack;
    stack.push_back(node);
    while(!stack.empty()){
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        visited[tmp->id] = true;
        check_val += controls[tmp->id];
        
        std::set<RedCapNode*>& nbrs = tmp->neighbors;
        std::set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id != exclude_node->id &&
                visited.find(nn->id) == visited.end())// && // not add yet
            {
                stack.push_back(nn);
                visited[nn->id] = true;
            }
        }
    }
    
    return check_val >  control_thres;
}

set<int> SpanningTree::getSubTree(RedCapNode* a, RedCapNode* exclude_node)
{
    set<int> visited;
    list<RedCapNode*> stack;
    stack.push_back(a);
    while (!stack.empty()) {
        RedCapNode* tmp = stack.front();
        stack.pop_front();
        visited.insert(tmp->id);
        set<RedCapNode*>& nbrs = tmp->neighbors;
        set<RedCapNode*>::iterator it;
        for (it=nbrs.begin(); it!=nbrs.end(); it++) {
            RedCapNode* nn = *it;
            if (nn->id != exclude_node->id &&
                node_dict.find(nn->id) != node_dict.end() &&
                visited.find(nn->id) == visited.end())
            {
                stack.push_back(nn);
                visited.insert(nn->id);
            }
        }
    }
    return visited;
}

void SpanningTree::subSplit(int start, int end)
{
    // search best cut
    double hg = 0;
    RedCapEdge* best_e = NULL;
    SpanningTree* left_best=NULL;
    SpanningTree* right_best=NULL;
    
    bool is_first = true;
    for (int i=start; i<=end; i++) {
        RedCapEdge* e = edges[i];
        // remove i-th edge, there should be two sub-trees created (left, right)
        set<int> left_ids = getSubTree(e->a, e->b);
        set<int> right_ids = getSubTree(e->b, e->a);

        double left_ssd = computeSSD(left_ids);
        double right_ssd = computeSSD(right_ids);

        if (left_ssd == 0 || right_ssd == 0) {
            // can't be split-ted: e.g. only one node, or doesn't check control
            continue;
        }
        
        double hg_sub = left_ssd + right_ssd;
        if (is_first || (hg > 0 && hg_sub < hg) ) {
            hg = hg_sub;
            best_e = e;
            is_first = false;
        }
    }
    if (best_e && hg > 0) {
        if ( cand_trees.find(best_e) == cand_trees.end() ) {
            cand_trees[best_e] = hg;
            
        } else {
            if ( cand_trees[best_e] > hg) {
                cand_trees[best_e] = hg;
            }
        }
    }
}

bool SpanningTree::checkControl(set<int>& ids)
{
    if (controls == NULL) {
        return true;
    }
    
    double val = 0;
    set<int>::iterator it;
    for (it=ids.begin(); it!=ids.end(); it++) {
        val += controls[*it];
    }
    
    return val > control_thres;
}

SpanningTree* SpanningTree::GetLeftChild()
{
    return left_child;
}

SpanningTree* SpanningTree::GetRightChild()
{
    return right_child;
}

double SpanningTree::computeSSD(set<int>& ids)
{
    if (ids.size() <= 1) {
        return 0;
    }
    
    // sum of squared deviations (ssd)
    if (ssd_dict.find(ids) != ssd_dict.end()) {
        return ssd_dict[ids];
    }
    
    double sum_ssd = 0;
    if (controls != NULL && checkControl(ids) == false) {
        return 0; // doesn't satisfy control variable
    }
    
    int n_vars = data[0].size();
    int nn = ids.size();
    
    vector<double> avg(n_vars);
    if (avg_dict.find(ids) != avg_dict.end()) {
        avg = avg_dict[ids];
    }
    
    set<int>::iterator it;
    
    if (avg.empty()) {
        double _s = 0;
        for (int i=0; i<n_vars; i++) {
            _s = 0;
            for (it=ids.begin(); it!=ids.end(); it++) {
                _s += data[*it][i];
            }
            avg.push_back(_s/nn);
        }
        boost::mutex::scoped_lock scoped_lock(mutex);
        avg_dict[ids] = avg;
    }
    
    for (int i=0; i<n_vars; i++) {
        double delta = 0;
        double ssd = 0;
        for (it=ids.begin(); it!=ids.end(); it++) {
            delta = data[*it][i] - avg[i];
            ssd += delta * delta;
        }
        sum_ssd += ssd;
    }
   
    boost::mutex::scoped_lock scoped_lock(mutex);
    ssd_dict[ids]  = sum_ssd;
    
    return sum_ssd;
}

double SpanningTree::GetSSD()
{
    if (ssd < 0) {
        set<int> ids;
        unordered_map<int, bool>::iterator it;
        for (it = node_dict.begin(); it != node_dict.end(); it++) {
            ids.insert(it->first);
        }
        ssd =  computeSSD(ids);
    }
    return ssd;
}

void SpanningTree::save()
{
    vector<RedCapEdge*>::iterator eit;
    
    wxTextFile file("/Users/xun/Desktop/frequence.gwt");
    file.Create("/Users/xun/Desktop/frequence.gwt");
    file.Open("/Users/xun/Desktop/frequence.gwt");
    file.Clear();
    file.AddLine("0 9 redcap fid");
    
    for (eit=edges.begin(); eit!=edges.end(); eit++) {
        wxString line;
        line << (*eit)->a->id << " " << (*eit)->b->id << " 1" ;
        file.AddLine(line);
    }
    file.Write();
    file.Close();
}

////////////////////////////////////////////////////////////////////////////////
//
// AbstractRedcap
//
////////////////////////////////////////////////////////////////////////////////
AbstractRedcap::AbstractRedcap(const vector<vector<double> >& _distances,
                               const vector<vector<double> >& _data,
                               const vector<bool>& _undefs,
                               GalElement * _w)
: distances(_distances), data(_data), undefs(_undefs), w(_w)
{
    ssd_dict.clear();
    avg_dict.clear();
    
    mstree = NULL;
    num_obs = data.size();
    num_vars = data[0].size();
    
    first_order_dict.resize(num_obs);
    for (int i=0; i<num_obs; i++) {
        first_order_dict[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            first_order_dict[i][j] = false;
        }
    }
}

AbstractRedcap::~AbstractRedcap()
{
    if (mstree) {
        delete mstree;
    }
    for (int i=0; i<first_order_edges.size(); i++) {
        delete first_order_edges[i];
    }
    for (int i=0; i<all_nodes.size(); i++) {
        delete all_nodes[i];
    }
}

bool RedCapEdgeLess(RedCapEdge* a, RedCapEdge* b)
{
    return a->length < b->length;
}

bool RedCapEdgeLarger(RedCapEdge* a, RedCapEdge* b)
{
    return a->length > b->length;
}

void AbstractRedcap::init()
{
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) {
            continue;
        }
        RedCapNode* node = new RedCapNode(i, data[i]);
        all_nodes.push_back(node);
    }
    
    // create first_order_edges
    for (int i=0; i<num_obs; i++) {
        if (undefs[i]) {
            continue;
        }
        const vector<long>& nbrs = w[i].GetNbrs();
        const vector<double>& nbrs_w = w[i].GetNbrWeights();
        for (int j=0; j<nbrs.size(); j++) {
            int nbr = nbrs[j];
            if (undefs[nbr] || i == nbr) {
                continue;
            }
            if (checkFirstOrderEdge(i, nbr) == false) {
                double w = nbrs_w[j];
                RedCapNode* a = all_nodes[i];
                RedCapNode* b = all_nodes[nbr];
                RedCapEdge* e = new RedCapEdge(a, b, distances[a->id][b->id]);
                first_order_edges.push_back(e);
                first_order_dict[a->id][b->id] = true;
                first_order_dict[b->id][a->id] = true;
            }
        }
    }
    
    // init SpanningTree
    mstree = new SpanningTree(all_nodes, data, undefs, controls, control_thres);
    
    Clustering();
    
    mstree->save();
}

bool AbstractRedcap::checkFirstOrderEdge(int node_i, int node_j)
{
    if (first_order_dict[node_i][node_j] || first_order_dict[node_j][node_i]) {
        return true;
    }
    return false;
}

void AbstractRedcap::createFirstOrderEdges(vector<RedCapEdge*>& edges)
{
    
}

void AbstractRedcap::createFullOrderEdges(vector<RedCapEdge*>& edges)
{
    for (int i=0; i<num_obs; i++) {
        if (undefs[i])
            continue;
        for (int j=i+1; j<num_obs; j++) {
            if (i == j)
                continue;
            RedCapNode* a = all_nodes[i];
            RedCapNode* b = all_nodes[j];
            RedCapEdge* e = new RedCapEdge(a, b, distances[a->id][b->id]);
            edges.push_back(e);
        }
    }
}

bool AbstractRedcap::CheckSpanningTree()
{
    return true;
}

vector<vector<int> >& AbstractRedcap::GetRegions()
{
    return cluster_ids;
}

bool RedCapTreeLess(SpanningTree* a, SpanningTree* b)
{
    //return a->all_nodes_dict.size() < b->all_nodes_dict.size();
    return a->ssd < b->ssd;
}

void AbstractRedcap::Partitioning(int k)
{
    wxStopWatch sw;
    
    PriorityQueue sub_trees;
    sub_trees.push(mstree);
   
    vector<SpanningTree*> not_split_trees;
    
    while (!sub_trees.empty() && sub_trees.size() < k) {
        SpanningTree* tmp_tree = sub_trees.top();
        sub_trees.pop();
        tmp_tree->Split();
       
        SpanningTree* left_tree = tmp_tree->GetLeftChild();
        SpanningTree* right_tree = tmp_tree->GetRightChild();
    
        if (left_tree== NULL && right_tree ==NULL) {
            not_split_trees.push_back(tmp_tree);
            k = k -1;
            continue;
        }
        
        if (left_tree) {
            left_tree->GetSSD();
            sub_trees.push(left_tree);
        }
        if (right_tree) {
            right_tree->GetSSD();
            sub_trees.push(right_tree);
        }
    }
  
    cluster_ids.clear();
    
    for (int i = 0; i< not_split_trees.size(); i++)
        sub_trees.push(not_split_trees[i]);
    
    PriorityQueue::iterator begin = sub_trees.begin();
    PriorityQueue::iterator end = sub_trees.end();
    unordered_map<int, bool>::iterator node_it;
   
    for (PriorityQueue::iterator it = begin; it != end; ++it) {
        SpanningTree* _tree = *it;
        vector<int> cluster;
        for (node_it=_tree->node_dict.begin();
             node_it!=_tree->node_dict.end(); node_it++)
        {
            cluster.push_back(node_it->first);
        }
        cluster_ids.push_back(cluster);
    }
    
    wxString time = wxString::Format("The long running function took %ldms to execute", sw.Time());
}

////////////////////////////////////////////////////////////////////////////////
//
// RedCapCluster
//
////////////////////////////////////////////////////////////////////////////////
RedCapCluster::RedCapCluster(RedCapEdge* edge)
{
    node_dict[edge->a] = true;
    node_dict[edge->b] = true;
    
    node_id_dict[edge->a->id] = true;
    node_id_dict[edge->b->id] = true;
}

RedCapCluster::RedCapCluster(RedCapNode* node)
{
    // single node cluster
    root = node;
    
    node_dict[node] = true;
    
    node_id_dict[node->id] = true;
}

RedCapCluster::~RedCapCluster()
{
}

int RedCapCluster::size()
{
    return node_id_dict.size();
}

bool RedCapCluster::IsConnectWith(RedCapCluster* c)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=node_dict.begin(); it!=node_dict.end(); it++) {
        if (c->Has(it->first))
            return true;
    }
    return false;
}

RedCapNode* RedCapCluster::GetNode(int i)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    it = node_dict.begin();
    std::advance(it, i);
    return it->first;
}

bool RedCapCluster::Has(RedCapNode* node)
{
    return node_id_dict.find(node->id) != node_id_dict.end();
}

bool RedCapCluster::Has(int id)
{
    return node_id_dict.find(id) != node_id_dict.end();
}

void RedCapCluster::AddNode(RedCapNode* node)
{
    node_dict[node] = true;
    node_id_dict[node->id] = true;
}

void RedCapCluster::Merge(RedCapCluster* cluster)
{
    unordered_map<RedCapNode*, bool>::iterator it;
    for (it=cluster->node_dict.begin(); it!=cluster->node_dict.end(); it++) {
        node_dict[it->first] = true;
        node_id_dict[it->first->id]  = true;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// RedCapClusterManager
//
////////////////////////////////////////////////////////////////////////////////
RedCapClusterManager::RedCapClusterManager()
{
}

RedCapClusterManager::~RedCapClusterManager()
{
    for (it=clusters_dict.begin(); it!=clusters_dict.end(); it++) {
        delete it->first;
    }
}

RedCapCluster* RedCapClusterManager::UpdateByAdd(RedCapEdge* edge)
{
    // updte clusters by adding edge
    RedCapCluster* c1 = getCluster(edge->a);
    RedCapCluster* c2 = getCluster(edge->b);
    
    if (c1 == c2)
        return c1;
    
    return mergeClusters(c1, c2);
}

bool RedCapClusterManager::HasCluster(RedCapCluster* c)
{
    return clusters_dict.find(c) != clusters_dict.end();
}



bool RedCapClusterManager::CheckConnectivity(RedCapEdge* edge, RedCapCluster** l, RedCapCluster** m)
{
    // check if edge connects two DIFFERENT clusters, if yes, then return l and m
    if (*l == 0 ) {
        *l = getCluster(edge->a);
    }
    if (*m == 0 ) {
        *m = getCluster(edge->b);
    }
   
    if (*l == *m) {
        return false;
    }
    
    if (((*l)->Has(edge->a) && (*m)->Has(edge->b)) ||
        ((*l)->Has(edge->b) && (*m)->Has(edge->a)) ) {
        return true;
    }
    
    return false;
}

RedCapCluster* RedCapClusterManager::getCluster(RedCapNode* node)
{
    for (it=clusters_dict.begin(); it!=clusters_dict.end(); it++) {
        RedCapCluster* cluster = it->first;
        if (cluster->Has(node)) {
            return cluster;
        }
    }
    return createCluster(node); // only one node
}

RedCapCluster* RedCapClusterManager::createCluster(RedCapNode *node)
{
    RedCapCluster* new_cluster = new RedCapCluster(node);
    clusters_dict[new_cluster] = true;
    return new_cluster;
}

RedCapCluster* RedCapClusterManager::mergeToCluster(RedCapNode* node, RedCapCluster* cluster)
{
    cluster->AddNode(node);
    return cluster;
}

RedCapCluster* RedCapClusterManager::mergeClusters(RedCapCluster* cluster1, RedCapCluster* cluster2)
{
    cluster1->Merge(cluster2);
  
    clusters_dict.erase(clusters_dict.find(cluster2));
    delete cluster2;
    
    return cluster1;
}

////////////////////////////////////////////////////////////////////////////////
//
// 1 FirstOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderSLKRedCap::FirstOrderSLKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement* w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderSLKRedCap::~FirstOrderSLKRedCap()
{
    
}

void FirstOrderSLKRedCap::Clustering()
{
    // step 1. sort edges based on length
    std::sort(first_order_edges.begin(), first_order_edges.end(), RedCapEdgeLess);
    
    // step 2.
    for (int i=0; i<first_order_edges.size(); i++) {
        RedCapEdge* edge = first_order_edges[i];
        
        // if an edge connects two different clusters, it is added to T,
        // and the two clusters are merged;
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        cm.UpdateByAdd(edge);
        
        mstree->AddEdge(edge);
        bool b_all_node_covered = mstree->IsFullyCovered();
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 2 FirstOrderALKRedCap
// The First-Order-ALK method also starts with the spatially contiguous graph G*. However, after each merge, the distance between the new cluster and every other cluster is recalculated. Therefore, edges that connect the new cluster and every other cluster are updated with new length values. Edges in G* are then re-sorted and re-evaluated from the beginning. The procedure stops when all objects are in one cluster. The algorithm is shown in figure 3. The complexity is O(n2log n) due to the sorting after each merge.
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderALKRedCap::FirstOrderALKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderALKRedCap::~FirstOrderALKRedCap()
{
    
}

/**
 *
 */
void FirstOrderALKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    for (int i=0; i<num_obs; i++) {
        cm.createCluster(all_nodes[i]);
    }
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    
    // 2. construct an nxn matrix avgDist to store distances between clusters
    // avgDist
    vector<vector<double> > avgDist(num_obs);
    for (int i=0; i<num_obs; i++) {
        avgDist[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            avgDist[i][j] = first_order_dict[i][j] ? distances[i][j] : 0;
        }
    }
    vector<vector<int> > numEdges(num_obs);
    for (int i=0; i<num_obs; i++) {
        numEdges[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            numEdges[i][j] = first_order_dict[i][j] ? 1 : 0;
        }
    }
    
    // 3. For each edge e in the sorted list (shortest first)
    while (!E.empty()) {
        // get shortest edge
        RedCapEdge* edge = E.back();
        E.pop_back();
        
        // If e connects two different clusters l, m, and e.length >= avgDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        int l_id = l->root->id;
        int m_id = m->root->id;
        
        if ( edge->length < avgDist[l_id][m_id] ) {
            continue;
        }

        // The following code never works
        /*
        // (1) find shortest edge e' in E that connnect cluster l and m
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, &l, &m)) {
                if (tmp_e->length < edge->length)
                    edge = tmp_e;
            }
        }
         */
        
        // (2) add e'to T ane merge m to l (l is now th new cluster)
        int n = cm.clusters_dict.size();
        l = cm.UpdateByAdd(edge);
        mstree->AddEdge(edge);

        
        // (3) for each cluster c that is not l
        //      update avgDist(c, l) in E that connects c and l
        unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
        n = cm.clusters_dict.size();
        bool dist_changed = false;
        for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
            RedCapCluster* c = it->first;
            if (c != l) {
                int c_id = c->root->id;
                double nn =numEdges[c_id][l_id] + (numEdges[c_id][m_id]);
                if (nn > 0) {
                    double d = (avgDist[c_id][l_id] * numEdges[c_id][l_id] + avgDist[c_id][m_id] * numEdges[c_id][m_id]) / nn;
                    avgDist[c_id][l_id] = d;
                    avgDist[l_id][c_id] = d;
                    numEdges[c_id][l_id] = nn;
                    numEdges[l_id][c_id] = nn;
                    
                    // update e
                    for (int i=0; i<E.size(); i++) {
                        if ((c->Has(E[i]->a) && l->Has(E[i]->b)) ||
                            (c->Has(E[i]->b) && l->Has(E[i]->a)) ) {
                            E[i]->length = d;
                            dist_changed = true;
                        }
                    }
                }
            }
        }
        
        // (4) sort all edges in E and set e = shortest one in the list
        if (dist_changed) {
            sort(E.begin(), E.end(), RedCapEdgeLarger);
        }
        
        bool b_all_node_covered = mstree->IsFullyCovered();
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 3 FirstOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FirstOrderCLKRedCap::FirstOrderCLKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FirstOrderCLKRedCap::~FirstOrderCLKRedCap()
{
    
}

double FirstOrderCLKRedCap::getMaxDist(RedCapCluster* l, RedCapCluster* m)
{
    return maxDist[l->root->id][m->root->id];
}

void FirstOrderCLKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLarger);
    
    // 2. construct an nxn matrix maxDist to store distances between clusters
    // maxDist
    maxDist.resize(num_obs);
    for (int i=0; i<num_obs; i++) {
        maxDist[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            maxDist[i][j] = first_order_dict[i][j] ? distances[i][j] : 0;
        }
    }
    
    // 3. For each edge e in the sorted list (shortest first)
    while (!E.empty()) {
        // get shortest edge
        RedCapEdge* edge = E.back();
        E.pop_back();
        
        // If e connects two different clusters l, m, and e.length >= maxDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        int l_id = l->root->id;
        int m_id = m->root->id;
        
        if ( edge->length < maxDist[l_id][m_id] ) {
            continue;
        }
       
        // (1) find shortest edge e' in E that connnect cluster l and m
        /*
        int e_idx = -1;
        RedCapEdge* copy_e =  edge;
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, &l, &m)) {
                if (tmp_e->length < edge->length) {
                    edge = tmp_e;
                    e_idx = j;
                }
            }
        }
        */
        // (2) add e'to T ane merge m to l (l is now th new cluster) m be removed
        l = cm.UpdateByAdd(edge);
        mstree->AddEdge(edge);
        
        // (3) for each cluster c that is not l
        //      update maxDist(c, l) in E that connects c and l
        //      maxDist(c,l) = max( maxDist(c,l), maxDist(c,m) )
        unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
        for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
            RedCapCluster* c = it->first;
            if (c != l) {
                int c_id = c->root->id;
                maxDist[c_id][l_id] = max(maxDist[c_id][l_id], maxDist[c_id][m_id]);
                maxDist[l_id][c_id] = max(maxDist[c_id][l_id], maxDist[c_id][m_id]);
            }
        }
        bool b_all_node_covered = mstree->IsFullyCovered();
        // stop when all nodes are covered by this tree
        if (b_all_node_covered) {
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 4 FullOrderSLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderSLKRedCap::FullOrderSLKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderSLKRedCap::~FullOrderSLKRedCap()
{
    
}

void FullOrderSLKRedCap::Clustering()
{
    // make a copy of first_first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    // create a full_order_deges
    vector<RedCapEdge*> FE;
    createFullOrderEdges(FE);
    
    vector<vector<bool> > conn(num_obs);
    for (int i=0; i< num_obs; i++) {
        conn[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            conn[i][j] = first_order_dict[i][j];
        }
    }
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLess);
    std::sort(FE.begin(), FE.end(), RedCapEdgeLess);
    
    // 2 Repeat until all nodes in MSTree
    int n_fe = FE.size();
    int idx = 0;
    while (idx < n_fe) {
        // e is the i-th edge in E
        RedCapEdge* edge = FE[idx++];

        // If e connects two different clusters l, m, and C(l,m) = contiguity
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        
        int l_id = l->root->id;
        int m_id = m->root->id;
        if (conn[l_id][m_id] == false) {
            continue;
        }
        
        // (1) find shortest edge e' in E that connnect cluster l and m
        int e_idx = -1;
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, &l, &m)) {
                edge = tmp_e;
                e_idx = j;
                break;
            }
        }
        
        if (e_idx < 0) {
            continue;
        }
        
        // (2) add e'to T
        mstree->AddEdge(edge);
        E.erase(E.begin()+e_idx);
    
        // (3) for each existing cluster c, c <> l , c <> m
        // Set C(c,l) = 1 if C(c,l) == 1 or C(c,m) == 1
        unordered_map<int, bool>::iterator lit;
        unordered_map<int, bool>::iterator mit;
        for (lit=l->node_id_dict.begin(); lit!=l->node_id_dict.end(); lit++) {
            for (mit=m->node_id_dict.begin(); mit!=m->node_id_dict.end(); mit++) {
                conn[lit->first][mit->first] = true;
                conn[mit->first][lit->first] = true;
            }
        }
        
        // (4) Merge cluster m to cluster l (l is now th new cluster)
        l = cm.UpdateByAdd(edge);

        // (5) reset i = 0
        idx  = 0;
        
        // stop when all nodes are covered by this tree
        bool b_all_node_covered = mstree->IsFullyCovered();
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 5 FullOrderALKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderALKRedCap::FullOrderALKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs,  GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderALKRedCap::~FullOrderALKRedCap()
{
    
}

void FullOrderALKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    for (int i=0; i<num_obs; i++) {
        cm.createCluster(all_nodes[i]);
    }
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLess);

    // 2. construct an nxn matrix avgDist to store distances between clusters
    // avgDist
    vector<vector<double> > avgDist(num_obs);
    for (int i=0; i<num_obs; i++) {
        avgDist[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            avgDist[i][j] = distances[i][j];
        }
    }
    vector<vector<int> > numEdges(num_obs);
    for (int i=0; i<num_obs; i++) {
        numEdges[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            numEdges[i][j] = 1;
        }
    }
    
    // 3. For each edge e in the sorted list (shortest first)
    while (!E.empty()) {
        // get shortest edge
        RedCapEdge* edge = E.back();
        E.pop_back();
        
        // If e connects two different clusters l, m, and e.length >= avgDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        int l_id = l->root->id;
        int m_id = m->root->id;
        
        if ( edge->length < avgDist[l_id][m_id] ) {
            continue;
        }
        
        // The following code never works
        /*
         // (1) find shortest edge e' in E that connnect cluster l and m
         for (int j=0; j<E.size(); j++) {
         RedCapEdge* tmp_e = E[j];
         if (cm.CheckConnectivity(tmp_e, &l, &m)) {
         if (tmp_e->length < edge->length)
         edge = tmp_e;
         }
         }
         */
        
        // (2) add e'to T ane merge m to l (l is now th new cluster)
        int n = cm.clusters_dict.size();
        l = cm.UpdateByAdd(edge);
        mstree->AddEdge(edge);
        
        
        // (3) for each cluster c that is not l
        //      update avgDist(c, l) in E that connects c and l
        unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
        n = cm.clusters_dict.size();
        bool dist_changed = false;
        for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
            RedCapCluster* c = it->first;
            if (c != l) {
                int c_id = c->root->id;
                double nn =numEdges[c_id][l_id] + (numEdges[c_id][m_id]);
                if (nn > 0) {
                    double d = (avgDist[c_id][l_id] * numEdges[c_id][l_id] + avgDist[c_id][m_id] * numEdges[c_id][m_id]) / nn;
                    avgDist[c_id][l_id] = d;
                    avgDist[l_id][c_id] = d;
                    numEdges[c_id][l_id] = nn;
                    numEdges[l_id][c_id] = nn;
                    
                    // update e
                    for (int i=0; i<E.size(); i++) {
                        if ((c->Has(E[i]->a) && l->Has(E[i]->b)) ||
                            (c->Has(E[i]->b) && l->Has(E[i]->a)) ) {
                            E[i]->length = d;
                            dist_changed = true;
                        }
                    }
                }
            }
        }
        
        // (4) sort all edges in E and set e = shortest one in the list
        if (dist_changed) {
            sort(E.begin(), E.end(), RedCapEdgeLarger);
        }
        
        bool b_all_node_covered = mstree->IsFullyCovered();
        
        // stop when all nodes are covered by this tree
        if (b_all_node_covered)
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// 6 FullOrderCLKRedCap
//
////////////////////////////////////////////////////////////////////////////////
FullOrderCLKRedCap::FullOrderCLKRedCap(const vector<vector<double> >& _distances, const vector<vector<double> >& _data, const vector<bool>& _undefs, GalElement * w, double* _controls, double _control_thres)
: AbstractRedcap(_distances, _data, _undefs, w)
{
    controls = _controls;
    control_thres = _control_thres;
    init();
}

FullOrderCLKRedCap::~FullOrderCLKRedCap()
{
    
}


void FullOrderCLKRedCap::Clustering()
{
    // make a copy of first_order_edges
    vector<RedCapEdge*> E = first_order_edges;
    for (int i=0; i<num_obs; i++) {
        cm.createCluster(all_nodes[i]);
    }
    
    // create a full_order_deges
    vector<RedCapEdge*> FE;
    createFullOrderEdges(FE);
    
    // 1. sort edges based on length
    std::sort(E.begin(), E.end(), RedCapEdgeLess);
    std::sort(FE.begin(), FE.end(), RedCapEdgeLarger);
    
    // 2. construct an nxn matrix maxDist to store distances between clusters
    // maxDist
    maxDist.resize(num_obs);
    for (int i=0; i<num_obs; i++) {
        maxDist[i].resize(num_obs);
        for (int j=0; j<num_obs; j++) {
            maxDist[i][j] = distances[i][j];
        }
    }
    
    // 3. For each edge e in the sorted list (shortest first)
    int idx = 0;
    int n_fe = FE.size();
    //while (idx < n_fe) {
    while (!FE.empty()) {
        // get shortest edge
        RedCapEdge* edge = FE.back();
        FE.pop_back();
        // get shortest edge
        //RedCapEdge* edge = FE[idx++];
        
        // If e connects two different clusters l, m, and e.length >= maxDist(l,m)
        RedCapCluster* l = NULL;
        RedCapCluster* m = NULL;
        if (!cm.CheckConnectivity(edge, &l, &m) ) {
            continue;
        }
        int l_id = l->root->id;
        int m_id = m->root->id;

        if ( edge->length < maxDist[l_id][m_id] ) {
            continue;
        }
        
        // (1) find shortest edge e' in E that connnect cluster l and m
        int e_idx = -1;
        for (int j=0; j<E.size(); j++) {
            RedCapEdge* tmp_e = E[j];
            if (cm.CheckConnectivity(tmp_e, &l, &m)) {
                edge = tmp_e;
                e_idx = j;
                break;
            }
        }
        
        if (e_idx < 0) {
            continue;
        }
        
        // (2) add e'to T ane merge m to l (l is now th new cluster) m be removed
        l = cm.UpdateByAdd(edge);
        mstree->AddEdge(edge);
        E.erase(E.begin()+e_idx);

        // (3) for each cluster c that is not l
        //      update maxDist(c, l) in E that connects c and l
        //      maxDist(c,l) = max( maxDist(c,l), maxDist(c,m) )
        bool dist_changed = false;
        unordered_map<RedCapCluster*, bool>::iterator it; // cluster iterator
        for (it=cm.clusters_dict.begin(); it!=cm.clusters_dict.end(); it++) {
            RedCapCluster* c = it->first;
            if (c != l) {
                int c_id = c->root->id;
                double d = max(maxDist[c_id][l_id], maxDist[c_id][m_id]);
                maxDist[c_id][l_id] = d;
                maxDist[l_id][c_id] = d;
            }
        }
        
        bool b_all_node_covered = mstree->IsFullyCovered();
        // stop when all nodes are covered by this tree
        if (b_all_node_covered) {
            break;
        }
    }
}
