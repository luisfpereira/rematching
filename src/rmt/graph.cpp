/**
 * @file        graph.cpp
 * 
 * @brief       Impllements rmt::Graph.
 * 
 * @author      Filippo Maggioli\n
 *              (maggioli@di.uniroma1.it, maggioli.filippo@gmail.com)\n
 *              Sapienza, University of Rome - Department of Computer Science
 * 
 * @date        2023-07-17
 */
#include <rmt/graph.hpp>
#include <set>
#include <queue>
#include <algorithm>


using namespace rmt;

typedef std::pair<int, int> Edge;  // Mesh edges


Graph::Graph(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F)
{
    int nVerts = V.rows();
    m_Verts.resize(nVerts);
    for (int i = 0; i < nVerts; ++i)
        m_Verts[i] = V.row(i).segment<3>(0);

    // std::set<Edge> Edges;
    std::vector<Edge> Edges;
    Edges.reserve(6 * F.rows());
    int nTris = F.rows();
    for (int i = 0; i < nTris; ++i)
    {
        Edges.emplace_back(std::min(F(i, 0), F(i, 1)), std::max(F(i, 0), F(i, 1)));
        Edges.emplace_back(std::min(F(i, 1), F(i, 2)), std::max(F(i, 1), F(i, 2)));
        Edges.emplace_back(std::min(F(i, 2), F(i, 0)), std::max(F(i, 2), F(i, 0)));

        Edges.emplace_back(std::max(F(i, 0), F(i, 1)), std::min(F(i, 0), F(i, 1)));
        Edges.emplace_back(std::max(F(i, 1), F(i, 2)), std::min(F(i, 1), F(i, 2)));
        Edges.emplace_back(std::max(F(i, 2), F(i, 0)), std::min(F(i, 2), F(i, 0)));
    }
    std::sort(Edges.begin(), Edges.end());
    auto EEnd = std::unique(Edges.begin(), Edges.end());

    m_Idxs.resize(nVerts + 1);
    // m_Adjs.reserve(Edges.size());
    m_Adjs.reserve(std::distance(Edges.begin(), EEnd) + 1);
    int CurNode = 0;
    Eigen::Vector3d CurVert = m_Verts[0];
    for (auto it = Edges.begin(); it != EEnd; it++)
    {
        if (it->first != CurNode)
        {
            CurNode++;
            m_Idxs[CurNode] = m_Adjs.size();
            CurVert = m_Verts[CurNode];
        }

        int ONode = it->second;
        Eigen::Vector3d OVert = m_Verts[ONode];
        m_Adjs.emplace_back(ONode, (CurVert - OVert).norm());
    }
    m_Idxs[CurNode + 1] = m_Adjs.size();
}

Graph::Graph(const Eigen::MatrixXd& V, const std::vector<std::pair<int, int>>& E)
{
    int nVerts = V.rows();
    m_Verts.resize(nVerts);
    for (int i = 0; i < nVerts; ++i)
        m_Verts[i] = V.row(i).segment<3>(0);

    std::set<Edge> Edges;
    int nEdges = E.size();
    for (int i = 0; i < nEdges; ++i)
    {
        Edges.emplace(std::min(E[i].first, E[i].second), std::max(E[i].first, E[i].second));
        Edges.emplace(std::max(E[i].first, E[i].second), std::min(E[i].first, E[i].second));
    }

    m_Idxs.resize(nVerts + 1);
    m_Adjs.reserve(Edges.size());
    int CurNode = 0;
    Eigen::Vector3d CurVert = m_Verts[0];
    for (auto it = Edges.begin(); it != Edges.end(); it++)
    {
        if (it->first != CurNode)
        {
            CurNode++;
            m_Idxs[CurNode] = m_Adjs.size();
            CurVert = m_Verts[CurNode];
        }

        int ONode = it->second;
        Eigen::Vector3d OVert = m_Verts[ONode];
        m_Adjs.emplace_back(ONode, (CurVert - OVert).norm());
    }
    m_Idxs[CurNode + 1] = m_Adjs.size();
}

Graph::Graph(const Eigen::MatrixXd& V, const std::set<std::pair<int, int>>& E)
{
    int nVerts = V.rows();
    m_Verts.resize(nVerts);
    for (int i = 0; i < nVerts; ++i)
        m_Verts[i] = V.row(i).segment<3>(0);

    std::set<Edge> Edges;
    int nEdges = E.size();
    for (auto it = E.begin(); it != E.end(); it++)
    {
        Edges.emplace(std::min(it->first, it->second), std::max(it->first, it->second));
        Edges.emplace(std::max(it->first, it->second), std::min(it->first, it->second));
    }

    m_Idxs.resize(nVerts + 1);
    m_Adjs.reserve(Edges.size());
    int CurNode = 0;
    Eigen::Vector3d CurVert = m_Verts[0];
    for (auto it = Edges.begin(); it != Edges.end(); it++)
    {
        if (it->first != CurNode)
        {
            CurNode++;
            m_Idxs[CurNode] = m_Adjs.size();
            CurVert = m_Verts[CurNode];
        }

        int ONode = it->second;
        Eigen::Vector3d OVert = m_Verts[ONode];
        m_Adjs.emplace_back(ONode, (CurVert - OVert).norm());
    }
    m_Idxs[CurNode + 1] = m_Adjs.size();
}


Graph::Graph(const Graph& G)
{
    m_Verts = G.m_Verts;
    m_Idxs = G.m_Idxs;
    m_Adjs = G.m_Adjs;
}

Graph& Graph::operator=(const Graph& G)
{
    m_Verts = G.m_Verts;
    m_Idxs = G.m_Idxs;
    m_Adjs = G.m_Adjs;

    return *this;
}

Graph::Graph(Graph&& G)
{
    m_Verts = std::move(G.m_Verts);
    m_Idxs = std::move(G.m_Idxs);
    m_Adjs = std::move(G.m_Adjs);
}

Graph& Graph::operator=(Graph&& G)
{
    m_Verts = std::move(G.m_Verts);
    m_Idxs = std::move(G.m_Idxs);
    m_Adjs = std::move(G.m_Adjs);

    return *this;
}

Graph::~Graph() { }



int Graph::NumVertices() const { return m_Verts.size(); }
int Graph::NumEdges() const { return m_Adjs.size() / 2; }
int Graph::NumAdjacents(int i) const { return m_Idxs[i + 1] - m_Idxs[i]; }

const Eigen::Vector3d& Graph::GetVertex(int i) const { return m_Verts[i]; }
const std::vector<Eigen::Vector3d>& Graph::GetVertices() const { return m_Verts; }

const WEdge& Graph::GetAdjacent(int node_i, int adj_i) const
{
    return m_Adjs[m_Idxs[node_i] + adj_i];
}


std::vector<int> Graph::ConnectedComponents() const
{
    std::vector<int> CC;
    CC.resize(NumVertices(), 0);
    int CurCC = 0;

    Eigen::VectorXi Visited;
    Visited.resize(NumVertices());
    Visited.setZero();

    while (Visited.sum() < NumVertices())
    {
        int Root = 0;
        for (int i = 0; i < NumVertices(); ++i)
        {
            if (Visited[i] == 0)
            {
                Root = i;
                break;
            }
        }

        std::queue<int> Q;
        Q.emplace(Root);
        while (!Q.empty())
        {
            int N = Q.front();
            Q.pop();
            
            if (Visited[N] != 0)
                continue;

            CC[N] = CurCC;
            Visited[N] = 1;
            int DegN = NumAdjacents(N);
            for (int jj = 0; jj < DegN; ++jj)
            {
                int j = GetAdjacent(N, jj).first;
                Q.emplace(j);
            }
        }

        CurCC += 1;
    }

    return CC;
}