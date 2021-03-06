
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "../geometry/halfedge.h"
#include "debug.h"
#include <iostream>
#include <sstream>

/* Note on local operation return types:

    The local operations all return a std::optional<T> type. This is used so that your
    implementation can signify that it does not want to perform the operation for
    whatever reason (e.g. you don't want to allow the user to erase the last vertex).

    An optional can have two values: std::nullopt, or a value of the type it is
    parameterized on. In this way, it's similar to a pointer, but has two advantages:
    the value it holds need not be allocated elsewhere, and it provides an API that
    forces the user to check if it is null before using the value.

    In your implementaiton, if you have successfully performed the operation, you can
    simply return the required reference:

            ... collapse the edge ...
            return collapsed_vertex_ref;

    And if you wish to deny the operation, you can return the null optional:

            return std::nullopt;

    Note that the stubs below all reject their duties by returning the null optional.
*/

/*
    This method should replace the given vertex and all its neighboring
    edges and faces with a single face, returning the new face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_vertex(Halfedge_Mesh::VertexRef v) {

    (void)v;
    return std::nullopt;
}

/*
    This method should erase the given edge and return an iterator to the
    merged face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_edge(Halfedge_Mesh::EdgeRef e) {

    (void)e;
    return std::nullopt;
}

/*
    This method should collapse the given edge and return an iterator to
    the new vertex created by the collapse.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_edge(Halfedge_Mesh::EdgeRef e) {
    // don't collapse edges that will result with multiple edges with the same
    // vertices and no way of merging (super edge case).
    auto can_collapse_edge = [=](EdgeRef e) {
        // A line not connected to anything can't be collapsed
        if (e->halfedge()->twin() == e->halfedge()->next()) {
            return false;
        }
        std::unordered_set<VertexRef> seen;
        std::unordered_set<FaceRef> whitelisted_faces;
        whitelisted_faces.insert(e->halfedge()->face());
        whitelisted_faces.insert(e->halfedge()->twin()->face());

        std::vector<HalfedgeRef> hs{e->halfedge(), e->halfedge()->twin()};
        for (HalfedgeRef h : hs) {
            HalfedgeRef h_start =  h;
            do {
                VertexRef other = h->twin()->vertex();
                assert(h->vertex() == h_start->vertex());
                FaceRef f1 = h->face();
                FaceRef f2 = h->twin()->face();
                if (seen.find(other) != seen.end()
                    && whitelisted_faces.find(f1) == whitelisted_faces.end()
                    && whitelisted_faces.find(f2) == whitelisted_faces.end()) {
                    return false;
                }
                seen.insert(other);
                h = h->twin()->next();
            } while (h != h_start);
        }
        return true;
    };

    if (!can_collapse_edge(e)) {
        std::cout << "Can't collapse edge due to weird mesh result" << std::endl;
        return std::nullopt;
    }

    VertexRef v = new_vertex();
    v->halfedge() = e->halfedge()->next();
    v->pos = e->center();

    auto process_outbound_edges = [=](HalfedgeRef h) {
        HalfedgeRef h_start = h;
        while (h->twin()->next() != h_start) {
            // std::cout << h_start->id() << " " << h->id() << std::endl;
            h = h->twin()->next();
            assert(h->vertex() == h_start->vertex());
            h->vertex() = v;
        }

        h = h_start;
        while (h->twin()->next() != h_start) {
            h = h->twin()->next();
            assert(h->vertex() != h_start->vertex());
        }

        h = h_start;
        do {
            HalfedgeRef next = h->next();
            // if this halfedge's next was h_start, replace with h_start's next
            if (next== h_start) {
                h->next() = h_start->next();
                break;
            }
            h = next;
        } while (h != h_start);

        // give face new halfedges if it was the one we're planning to delete
        h_start->face()->halfedge() = h_start->next();

        // if face now has only 2 edges remove face
        HalfedgeRef h1 = h_start->next();
        if (h1 == h1->next()->next() && h1->edge() != h1->next()->edge()) {
            h1->vertex()->halfedge() = h1->next()->twin();
            h1->next()->vertex()->halfedge() = h1->twin();
            h1->twin()->twin() = h1->next()->twin();
            h1->next()->twin()->twin() = h1->twin();
            h1->twin()->edge() = h1->next()->edge();
            h1->twin()->edge()->halfedge() = h1->twin();

            // reassign v halfedge in case we just deleted it via merge
            v->halfedge() = h1->next()->twin();
            erase(h1->face());
            erase(h1->edge());
            erase(h1->next());
            erase(h1);
        }

        // delete halfedge
        erase(h_start->vertex());
        erase(h_start);
    };

    process_outbound_edges(e->halfedge());
    process_outbound_edges(e->halfedge()->twin());
    erase(e);

    return v;
}

/*
    This method should collapse the given face and return an iterator to
    the new vertex created by the collapse.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_face(Halfedge_Mesh::FaceRef f) {

    (void)f;
    return std::nullopt;
}

/*
    This method should flip the given edge and return an iterator to the
    flipped edge.
*/
std::optional<Halfedge_Mesh::EdgeRef> Halfedge_Mesh::flip_edge(Halfedge_Mesh::EdgeRef e) {
    if (e->on_boundary()) {
        return std::nullopt;
    }

    HalfedgeRef h0 = e->halfedge();
    HalfedgeRef h1 = e->halfedge()->twin();
    HalfedgeRef h2 = h0->next();
    HalfedgeRef h3 = h1->next();
    HalfedgeRef h4 = h0;
    do {
        h4 = h4->next();
    } while (h4->next() != h0);
    HalfedgeRef h5 = h1;
    do {
        h5 = h5->next();
    } while (h5->next() != h1);
    HalfedgeRef h6 = h2->next();
    HalfedgeRef h7 = h3->next();

    VertexRef v0 = h0->next()->vertex();
    VertexRef v1 = h1->next()->vertex();
    VertexRef v2 = h0->next()->next()->vertex();
    VertexRef v3 = h1->next()->next()->vertex();

    FaceRef f0 = h0->face();
    FaceRef f1 = h1->face();

    h0->vertex() = v3;
    h0->next() = h6;
    h1->vertex() = v2;
    h1->next() = h7;
    h2->next() = h1;
    h2->face() = f1;
    h3->next() = h0;
    h3->face() = f0;
    h4->next() = h3;
    h5->next() = h2;
    
    v0->halfedge() = h2;
    v1->halfedge() = h3;

    f0->halfedge() = h0;
    f1->halfedge() = h1;

    return e;
}

/*
    This method should split the given edge and return an iterator to the
    newly inserted vertex. The halfedge of this vertex should point along
    the edge that was split, rather than the new edges.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::split_edge(Halfedge_Mesh::EdgeRef e) {
    // if we're not dealing with triangles, don't do this operation
    if (e->halfedge()->next()->next()->next() != e->halfedge() ||
        e->halfedge()->twin()->next()->next()->next() != e->halfedge()->twin()) {
        return std::nullopt;
    }

    // lol look at your goodnotes for the reasoning...hard to grep looking at
    // code, but convention is to opt for lowest number.
    VertexRef v1 = e->halfedge()->vertex();
    VertexRef v2 = e->halfedge()->twin()->vertex();
    VertexRef v3 = e->halfedge()->next()->next()->vertex();
    VertexRef v4 = e->halfedge()->twin()->next()->next()->vertex();
    VertexRef v5 = new_vertex();

    HalfedgeRef h1 = e->halfedge();
    HalfedgeRef h2 = h1->next();
    HalfedgeRef h3 = h2->next();
    HalfedgeRef h4 = h1->twin();
    HalfedgeRef h5 = h4->next();
    HalfedgeRef h6 = h5->next();
    HalfedgeRef h7 = new_halfedge();
    HalfedgeRef h8 = new_halfedge();
    HalfedgeRef h9 = new_halfedge();
    HalfedgeRef h10 = new_halfedge();
    HalfedgeRef h11 = new_halfedge();
    HalfedgeRef h12 = new_halfedge();

    EdgeRef e1 = e;
    EdgeRef e2 = new_edge();
    EdgeRef e3 = new_edge();
    EdgeRef e4 = new_edge();

    FaceRef f1 = h1->face();
    FaceRef f2 = h4->face();
    FaceRef f3 = new_face();
    FaceRef f4 = new_face();

    v5->pos = e->center();
    v5->halfedge() = h12;

    h1->vertex() = v1;
    h1->edge() = e1;
    h1->twin() = h12;
    h1->next() = h7;
    h1->face() = f1;

    h2->vertex() = v2;
    h2->edge() = h2->edge();
    h2->twin() = h2->twin();
    h2->next() = h8;
    h2->face() = f2;

    h3->vertex() = v3;
    h3->edge() = h3->edge();
    h3->twin() = h3->twin();
    h3->next() = h1;
    h3->face() = f1;

    h4->vertex() = v2;
    h4->edge() = e2;
    h4->twin() = h9;
    h4->next() = h10;
    h4->face() = f3;

    h5->vertex() = v1;
    h5->edge() = h5->edge();
    h5->twin() = h5->twin();
    h5->next() = h11;
    h5->face() = f4;

    h6->vertex() = v4;
    h6->edge() = h6->edge();
    h6->twin() = h6->twin();
    h6->next() = h4;
    h6->face() = f3;

    h7->vertex() = v5;
    h7->edge() = e3;
    h7->twin() = h8;
    h7->next() = h3;
    h7->face() = f1;

    h8->vertex() = v3;
    h8->edge() = e3;
    h8->twin() = h7;
    h8->next() = h9;
    h8->face() = f2;

    h9->vertex() = v5;
    h9->edge() = e2;
    h9->twin() = h4;
    h9->next() = h2;
    h9->face() = f2;

    h10->vertex() = v5;
    h10->edge() = e4;
    h10->twin() = h11;
    h10->next() = h6;
    h10->face() = f3;

    h11->vertex() = v4;
    h11->edge() = e4;
    h11->twin() = h10;
    h11->next() = h12;
    h11->face() = f4;

    h12->vertex() = v5;
    h12->edge() = e1;
    h12->twin() = h1;
    h12->next() = h5;
    h12->face() = f4;

    e1->halfedge() = h1;
    e2->halfedge() = h4;
    e3->halfedge() = h7;
    e4->halfedge() = h10;

    f1->halfedge() = h1;
    f2->halfedge() = h2;
    f3->halfedge() = h4;
    f4->halfedge() = h5;

    return v5;
}

/* Note on the beveling process:

    Each of the bevel_vertex, bevel_edge, and bevel_face functions do not represent
    a full bevel operation. Instead, they should update the _connectivity_ of
    the mesh, _not_ the positions of newly created vertices. In fact, you should set
    the positions of new vertices to be exactly the same as wherever they "started from."

    When you click on a mesh element while in bevel mode, one of those three functions
    is called. But, because you may then adjust the distance/offset of the newly
    beveled face, we need another method of updating the positions of the new vertices.

    This is where bevel_vertex_positions, bevel_edge_positions, and
    bevel_face_positions come in: these functions are called repeatedly as you
    move your mouse, the position of which determins the normal and tangent offset
    parameters. These functions are also passed an array of the original vertex
    positions: for  bevel_vertex, it has one element, the original vertex position,
    for bevel_edge,  two for the two vertices, and for bevel_face, it has the original
    position of each vertex in halfedge order. You should use these positions, as well
    as the normal and tangent offset fields to assign positions to the new vertices.

    Finally, note that the normal and tangent offsets are not relative values - you
    should compute a particular new position from them, not a delta to apply.
*/

/*
    This method should replace the vertex v with a face, corresponding to
    a bevel operation. It should return the new face.  NOTE: This method is
    responsible for updating the *connectivity* of the mesh only---it does not
    need to update the vertex positions.  These positions will be updated in
    Halfedge_Mesh::bevel_vertex_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_vertex(Halfedge_Mesh::VertexRef v) {

    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."

    (void)v;
    return std::nullopt;
}

/*
    This method should replace the edge e with a face, corresponding to a
    bevel operation. It should return the new face. NOTE: This method is
    responsible for updating the *connectivity* of the mesh only---it does not
    need to update the vertex positions.  These positions will be updated in
    Halfedge_Mesh::bevel_edge_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_edge(Halfedge_Mesh::EdgeRef e) {

    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."

    (void)e;
    return std::nullopt;
}

/*
    This method should replace the face f with an additional, inset face
    (and ring of faces around it), corresponding to a bevel operation. It
    should return the new face.  NOTE: This method is responsible for updating
    the *connectivity* of the mesh only---it does not need to update the vertex
    positions. These positions will be updated in
    Halfedge_Mesh::bevel_face_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_face(Halfedge_Mesh::FaceRef f) {
    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."
    {
        // Create vertex copies
        std::vector<std::pair<VertexRef, VertexRef>> vertices;
        HalfedgeRef h_start = f->halfedge();
        HalfedgeRef h = h_start;
        do {
            VertexRef v_old = h->vertex();
            VertexRef v = new_vertex();
            v->pos = v_old->pos;
            vertices.push_back({v_old, v});
            h = h->next();
        } while (h != h_start);
        // a bit of a hack but avoids having to think of this as a circular
        // list.
        vertices.push_back(vertices.at(0));


        int num_edges = vertices.size() - 1;
        std::vector<std::vector<HalfedgeRef>> side_face_hs(num_edges);
        std::vector<HalfedgeRef> top_face_hs;
        int vi = 0;
        do {
            auto [v2, v0] = vertices.at(vi);
            auto [v3_, v1] = vertices.at(vi + 1);
            
            EdgeRef e = new_edge();
            HalfedgeRef h0 = new_halfedge();
            HalfedgeRef h1 = new_halfedge();

            v0->halfedge() = h0;
            f->halfedge() = h0;
            e->halfedge() = h0;
            
            h0->twin() = h1;
            h0->edge() = e;
            h0->vertex() = v0;
            h0->face() = f;
            h1->twin() = h0;
            h1->edge() = e;
            h1->vertex() = v1;
            h1->face() = f;

            EdgeRef e_side = new_edge();
            HalfedgeRef h0_side = new_halfedge();
            HalfedgeRef h1_side = new_halfedge();

            e_side->halfedge() = h0_side;
            h0_side->twin() = h1_side;
            h0_side->edge() = e_side;
            h0_side->vertex() = v0;
            h1_side->twin() = h0_side;
            h1_side->edge() = e_side;
            h1_side->vertex() = v2;

            top_face_hs.push_back(h0);
            side_face_hs.at(vi).push_back(h);
            side_face_hs.at(vi).push_back(h1);
            side_face_hs.at(vi).push_back(h0_side);
            int prev_vi = vi == 0 ? side_face_hs.size() - 1 : vi - 1;
            side_face_hs.at(prev_vi).push_back(h1_side);

            ++vi;
            h = h->next();
        } while (h != h_start);

        top_face_hs.push_back(top_face_hs.at(0));
        for (size_t ti = 0; ti < top_face_hs.size() - 1; ++ti) {
            top_face_hs.at(ti)->next() = top_face_hs.at(ti + 1);
        }

        for (const auto& face_hs : side_face_hs) {
            assert(face_hs.size() == 4);
            FaceRef f_side = new_face();
            for (HalfedgeRef h : face_hs) {
                h->face() = f_side;
                f_side->halfedge() = h;
                for (HalfedgeRef next : face_hs) {
                    if (next->vertex() == h->twin()->vertex()) {
                        h->next() = next;
                    }
                }
            }
        }

    }

    return f;
}

/*
    Compute new vertex positions for the vertices of the beveled vertex.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the original vertex position and its associated outgoing edge
    to compute a new vertex position along the outgoing edge.
*/
void Halfedge_Mesh::bevel_vertex_positions(const std::vector<Vec3>& start_positions,
                                           Halfedge_Mesh::FaceRef face, float tangent_offset) {

    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    (void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
}

/*
    Compute new vertex positions for the vertices of the beveled edge.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the orig array) to compute an offset vertex position.

    Note that there is a 1-to-1 correspondence between halfedges in
    newHalfedges and vertex positions
    in orig.  So, you can write loops of the form

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vector3D pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_edge_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset) {

    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    (void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
}

/*
    Compute new vertex positions for the vertices of the beveled face.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the start_positions array) to compute an offset vertex
    position.

    Note that there is a 1-to-1 correspondence between halfedges in
    new_halfedges and vertex positions
    in orig. So, you can write loops of the form

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vec3 pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_face_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset,
                                         float normal_offset) {

    if(flip_orientation) normal_offset = -normal_offset;
    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    // clamping
    if (tangent_offset > 1.5 || tangent_offset < -1.0) {
        return;
    }

    for(size_t i = 0; i < new_halfedges.size(); i++) {
        Vec3 start = start_positions.at(i);
        Vec3 normal_delta = face->normal() * normal_offset;
        Vec3 tangent = start - new_halfedges.at(i)->twin()->vertex()->pos;
        Vec3 tangent_delta = tangent * tangent_offset;
        Vec3 new_pos = start + normal_delta + tangent_delta;
        if (new_pos.valid()) {
            new_halfedges.at(i)->vertex()->pos = new_pos;
        }
    }
}

/*
    Splits all non-triangular faces into triangles.
*/
void Halfedge_Mesh::triangulate() {
    // no faces
    if (faces.begin() == faces.end()) {
        return;
    }
    auto last_face = faces.end();

    for (auto face = faces.begin(); face != last_face; ++face) {
        HalfedgeRef h_start = face->halfedge();
        HalfedgeRef ha = h_start;
        HalfedgeRef hb = ha->next();
        HalfedgeRef h = hb->next();
        while (h->next() != h_start) {
            EdgeRef e = new_edge();
            HalfedgeRef hc = new_halfedge();
            HalfedgeRef ha_next = new_halfedge();
            FaceRef f = new_face();
            e->halfedge() = hc;
            ha->next() = hb;
            ha->face() = f;
            hb->next() = hc;
            hb->face() = f;
            
            hc->next() = ha;
            hc->face() = f;
            hc->twin() = ha_next;
            hc->vertex() = h->vertex();
            hc->edge() = e;
            
            ha_next->next() = h;
            ha_next->face() = h->face();
            ha_next->twin() = hc;
            ha_next->vertex() = h_start->vertex();
            ha_next->edge() = e;

            f->halfedge() = hb;

            ha = ha_next;
            hb = h;
            h = h->next();
        }
        // handle last edge
        h->next() = ha;
        face->halfedge() = h;
    }
}

/* Note on the quad subdivision process:

        Unlike the local mesh operations (like bevel or edge flip), we will perform
        subdivision by splitting *all* faces into quads "simultaneously."  Rather
        than operating directly on the halfedge data structure (which as you've
        seen is quite difficult to maintain!) we are going to do something a bit nicer:
           1. Create a raw list of vertex positions and faces (rather than a full-
              blown halfedge mesh).
           2. Build a new halfedge mesh from these lists, replacing the old one.
        Sometimes rebuilding a data structure from scratch is simpler (and even
        more efficient) than incrementally modifying the existing one.  These steps are
        detailed below.

  Step I: Compute the vertex positions for the subdivided mesh.
        Here we're going to do something a little bit strange: since we will
        have one vertex in the subdivided mesh for each vertex, edge, and face in
        the original mesh, we can nicely store the new vertex *positions* as
        attributes on vertices, edges, and faces of the original mesh. These positions
        can then be conveniently copied into the new, subdivided mesh.
        This is what you will implement in linear_subdivide_positions() and
        catmullclark_subdivide_positions().

  Steps II-IV are provided (see Halfedge_Mesh::subdivide()), but are still detailed
  here:

  Step II: Assign a unique index (starting at 0) to each vertex, edge, and
        face in the original mesh. These indices will be the indices of the
        vertices in the new (subdivided mesh).  They do not have to be assigned
        in any particular order, so long as no index is shared by more than one
        mesh element, and the total number of indices is equal to V+E+F, i.e.,
        the total number of vertices plus edges plus faces in the original mesh.
        Basically we just need a one-to-one mapping between original mesh elements
        and subdivided mesh vertices.

  Step III: Build a list of quads in the new (subdivided) mesh, as tuples of
        the element indices defined above. In other words, each new quad should be
        of the form (i,j,k,l), where i,j,k and l are four of the indices stored on
        our original mesh elements.  Note that it is essential to get the orientation
        right here: (i,j,k,l) is not the same as (l,k,j,i).  Indices of new faces
        should circulate in the same direction as old faces (think about the right-hand
        rule).

  Step IV: Pass the list of vertices and quads to a routine that clears
        the internal data for this halfedge mesh, and builds new halfedge data from
        scratch, using the two lists.
*/

/*
    Compute new vertex positions for a mesh that splits each polygon
    into quads (by inserting a vertex at the face midpoint and each
    of the edge midpoints).  The new vertex positions will be stored
    in the members Vertex::new_pos, Edge::new_pos, and
    Face::new_pos.  The values of the positions are based on
    simple linear interpolation, e.g., the edge midpoints and face
    centroids.
*/
void Halfedge_Mesh::linear_subdivide_positions() {

    // For each vertex, assign Vertex::new_pos to
    // its original position, Vertex::pos.
    for (auto& vertex : vertices) {
        vertex.new_pos = vertex.pos;
    }
    // For each edge, assign the midpoint of the two original
    // positions to Edge::new_pos.
    for (auto& edge : edges) {
        edge.new_pos = edge.center();
    }

    // For each face, assign the centroid (i.e., arithmetic mean)
    // of the original vertex positions to Face::new_pos. Note
    // that in general, NOT all faces will be triangles!
    for (auto& face : faces) {
        face.new_pos = face.center();
    }
}

/*
    Compute new vertex positions for a mesh that splits each polygon
    into quads (by inserting a vertex at the face midpoint and each
    of the edge midpoints).  The new vertex positions will be stored
    in the members Vertex::new_pos, Edge::new_pos, and
    Face::new_pos.  The values of the positions are based on
    the Catmull-Clark rules for subdivision.

    Note: this will only be called on meshes without boundary
*/
void Halfedge_Mesh::catmullclark_subdivide_positions() {

    // The implementation for this routine should be
    // a lot like Halfedge_Mesh:linear_subdivide_positions:(),
    // except that the calculation of the positions themsevles is
    // slightly more involved, using the Catmull-Clark subdivision
    // rules. (These rules are outlined in the Developer Manual.)

    // Faces
    for (auto& face : faces) {
        face.new_pos = face.center();
    }
    // Edges
    for (auto& edge : edges) {
        Vec3 endpoint1 = edge.halfedge()->vertex()->pos;
        Vec3 endpoint2 = edge.halfedge()->twin()->vertex()->pos;
        FaceRef f1 = edge.halfedge()->face();
        FaceRef f2 = edge.halfedge()->twin()->face();
        edge.new_pos = (endpoint1 + endpoint2 + f1->new_pos + f2->new_pos) / 4;
    }
    // Vertices
    for (auto& vertex : vertices) {
        int n = 0;
        Vec3 face_sum;
        Vec3 edge_sum;
        HalfedgeRef h_start = vertex.halfedge();
        HalfedgeRef h = h_start;
        do {
            ++n;
            face_sum += h->face()->new_pos;
            edge_sum += h->edge()->new_pos;
            h = h->twin()->next();
        } while (h != h_start);
        Vec3 Q = face_sum / n;
        Vec3 R = edge_sum / n;
        Vec3 S = vertex.pos;
        vertex.new_pos = (Q + 2 * R + (n - 3) * S) / n;
    }
}

/*
        This routine should increase the number of triangles in the mesh
        using Loop subdivision. Note: this is will only be called on triangle meshes.
*/
void Halfedge_Mesh::loop_subdivide() {

    // Compute new positions for all the vertices in the input mesh, using
    // the Loop subdivision rule, and store them in Vertex::new_pos.
    // -> At this point, we also want to mark each vertex as being a vertex of the
    //    original mesh. Use Vertex::is_new for this.
    // -> Next, compute the updated vertex positions associated with edges, and
    //    store it in Edge::new_pos.
    // -> Next, we're going to split every edge in the mesh, in any order.  For
    //    future reference, we're also going to store some information about which
    //    subdivided edges come from splitting an edge in the original mesh, and
    //    which edges are new, by setting the flat Edge::is_new. Note that in this
    //    loop, we only want to iterate over edges of the original mesh.
    //    Otherwise, we'll end up splitting edges that we just split (and the
    //    loop will never end!)
    // -> Now flip any new edge that connects an old and new vertex.
    // -> Finally, copy the new vertex positions into final Vertex::pos.

    // Each vertex and edge of the original surface can be associated with a
    // vertex in the new (subdivided) surface.
    // Therefore, our strategy for computing the subdivided vertex locations is to
    // *first* compute the new positions
    // using the connectivity of the original (coarse) mesh; navigating this mesh
    // will be much easier than navigating
    // the new subdivided (fine) mesh, which has more elements to traverse.  We
    // will then assign vertex positions in
    // the new mesh based on the values we computed for the original mesh.

    // Compute updated positions for all the vertices in the original mesh, using
    // the Loop subdivision rule.

    // Next, compute the updated vertex positions associated with edges.

    // Next, we're going to split every edge in the mesh, in any order. For
    // future reference, we're also going to store some information about which
    // subdivided edges come from splitting an edge in the original mesh, and
    // which edges are new.
    // In this loop, we only want to iterate over edges of the original
    // mesh---otherwise, we'll end up splitting edges that we just split (and
    // the loop will never end!)

    // Finally, flip any new edge that connects an old and new vertex.

    // Copy the updated vertex positions to the subdivided mesh.
}

/*
    Isotropic remeshing. Note that this function returns success in a similar
    manner to the local operations, except with only a boolean value.
    (e.g. you may want to return false if this is not a triangle mesh)
*/
bool Halfedge_Mesh::isotropic_remesh() {

    // Compute the mean edge length.
    // Repeat the four main steps for 5 or 6 iterations
    // -> Split edges much longer than the target length (being careful about
    //    how the loop is written!)
    // -> Collapse edges much shorter than the target length.  Here we need to
    //    be EXTRA careful about advancing the loop, because many edges may have
    //    been destroyed by a collapse (which ones?)
    // -> Now flip each edge if it improves vertex degree
    // -> Finally, apply some tangential smoothing to the vertex positions

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate are called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

    return false;
}

/* Helper type for quadric simplification */
struct Edge_Record {
    Edge_Record() {
    }
    Edge_Record(std::unordered_map<Halfedge_Mesh::VertexRef, Mat4>& vertex_quadrics,
                Halfedge_Mesh::EdgeRef e)
        : edge(e) {

        // Compute the combined quadric from the edge endpoints.
        // -> Build the 3x3 linear system whose solution minimizes the quadric error
        //    associated with these two endpoints.
        // -> Use this system to solve for the optimal position, and store it in
        //    Edge_Record::optimal.
        // -> Also store the cost associated with collapsing this edge in
        //    Edge_Record::cost.
        Mat4 K = vertex_quadrics[e->halfedge()->vertex()] + vertex_quadrics[e->halfedge()->twin()->vertex()];
        Mat4 A = Mat4::axes(K.cols[0].xyz(), K.cols[1].xyz(), K.cols[2].xyz());
        Vec3 b = K[3].xyz() * -1;

        Mat4 inverse = A.inverse();
        bool invalid = !inverse.cols[0].valid() ||
                        !inverse.cols[1].valid() ||
                        !inverse.cols[2].valid() ||
                        !inverse.cols[3].valid();
        if (invalid) {
            std::cout << "invalid inverse" << std::endl;
            optimal = e->center();
        } else {
            optimal = e->center();
            (void) b;
            // optimal = inverse * b;
        }
        Vec4 x = Vec4{optimal, 1};
        cost = dot(Vec4{dot(x, K.cols[0]), dot(x, K.cols[1]), dot(x, K.cols[2]), dot(x, K.cols[3])}, x);
    }
    Halfedge_Mesh::EdgeRef edge;
    Vec3 optimal;
    float cost;
};

/* Comparison operator for Edge_Records so std::set will properly order them */
bool operator<(const Edge_Record& r1, const Edge_Record& r2) {
    if(r1.cost != r2.cost) {
        return r1.cost < r2.cost;
    }
    Halfedge_Mesh::EdgeRef e1 = r1.edge;
    Halfedge_Mesh::EdgeRef e2 = r2.edge;
    return &*e1 < &*e2;
}

/** Helper type for quadric simplification
 *
 * A PQueue is a minimum-priority queue that
 * allows elements to be both inserted and removed from the
 * queue.  Together, one can easily change the priority of
 * an item by removing it, and re-inserting the same item
 * but with a different priority.  A priority queue, for
 * those who don't remember or haven't seen it before, is a
 * data structure that always keeps track of the item with
 * the smallest priority or "score," even as new elements
 * are inserted and removed.  Priority queues are often an
 * essential component of greedy algorithms, where one wants
 * to iteratively operate on the current "best" element.
 *
 * PQueue is templated on the type T of the object
 * being queued.  For this reason, T must define a comparison
 * operator of the form
 *
 *    bool operator<( const T& t1, const T& t2 )
 *
 * which returns true if and only if t1 is considered to have a
 * lower priority than t2.
 *
 * Basic use of a PQueue might look
 * something like this:
 *
 *    // initialize an empty queue
 *    PQueue<myItemType> queue;
 *
 *    // add some items (which we assume have been created
 *    // elsewhere, each of which has its priority stored as
 *    // some kind of internal member variable)
 *    queue.insert( item1 );
 *    queue.insert( item2 );
 *    queue.insert( item3 );
 *
 *    // get the highest priority item currently in the queue
 *    myItemType highestPriorityItem = queue.top();
 *
 *    // remove the highest priority item, automatically
 *    // promoting the next-highest priority item to the top
 *    queue.pop();
 *
 *    myItemType nextHighestPriorityItem = queue.top();
 *
 *    // Etc.
 *
 *    // We can also remove an item, making sure it is no
 *    // longer in the queue (note that this item may already
 *    // have been removed, if it was the 1st or 2nd-highest
 *    // priority item!)
 *    queue.remove( item2 );
 *
 */
template<class T> struct PQueue {
    void insert(const T& item) {
        queue.insert(item);
    }
    void remove(const T& item) {
        if(queue.find(item) != queue.end()) {
            queue.erase(item);
        }
    }
    const T& top(void) const {
        return *(queue.begin());
    }
    void pop(void) {
        queue.erase(queue.begin());
    }
    size_t size() {
        return queue.size();
    }

    std::set<T> queue;
};

/*
    Mesh simplification. Note that this function returns success in a similar
    manner to the local operations, except with only a boolean value.
    (e.g. you may want to return false if you can't simplify the mesh any
    further without destroying it.)
*/
bool Halfedge_Mesh::simplify() {

    std::unordered_map<VertexRef, Mat4> vertex_quadrics;
    std::unordered_map<FaceRef, Mat4> face_quadrics;
    std::unordered_map<EdgeRef, Edge_Record> edge_records;
    PQueue<Edge_Record> edge_queue;

    // Compute initial quadrics for each face by simply writing the plane equation
    // for the face in homogeneous coordinates. These quadrics should be stored
    // in face_quadrics
    // -> Compute an initial quadric for each vertex as the sum of the quadrics
    //    associated with the incident faces, storing it in vertex_quadrics
    // -> Build a priority queue of edges according to their quadric error cost,
    //    i.e., by building an Edge_Record for each edge and sticking it in the
    //    queue. You may want to use the above PQueue<Edge_Record> for this.
    // -> Until we reach the target edge budget, collapse the best edge. Remember
    //    to remove from the queue any edge that touches the collapsing edge
    //    BEFORE it gets collapsed, and add back into the queue any edge touching
    //    the collapsed vertex AFTER it's been collapsed. Also remember to assign
    //    a quadric to the collapsed vertex, and to pop the collapsed edge off the
    //    top of the queue.

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate are called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

    for (FaceRef face = faces_begin(); face != faces_end(); ++face) {
        Vec3 N = face->normal();
        Vec3 p = face->center();
        float d = dot(N, p) * -1;
        Vec4 v(N, d);
        face_quadrics[face] = outer(v, v);
    }

    for (VertexRef vertex = vertices_begin(); vertex != vertices_end(); ++vertex) {
        Mat4 face_quadric_sum;
        HalfedgeRef h_start = vertex->halfedge();
        HalfedgeRef h = h_start;
        do {
            face_quadric_sum += face_quadrics[h->face()];
            h = h->twin()->next();
        } while (h != h_start);
        vertex_quadrics[vertex] = face_quadric_sum;
    }

    std::vector<EdgeRef> concerns(1);
    for (EdgeRef edge = edges_begin(); edge != edges_end(); ++edge) {
        edge_records[edge] = {vertex_quadrics, edge};
        edge_queue.insert(edge_records[edge]);
    }

    size_t num_deletions = (faces.size() / 4);
    constexpr size_t min_faces = 1;
    // the mesh is gone ...
    if (num_deletions >= faces.size() - min_faces) {
        return false;
    }

    auto get_touching_edges_to_vertex = [=](VertexRef v) {
        std::unordered_set<EdgeRef> touching_edges;
        HalfedgeRef h = v->halfedge();
        HalfedgeRef h_start = h;
        do {
            touching_edges.insert(h->edge());
            h = h->twin()->next();
        } while (h != h_start);
        return touching_edges;
    };

    // excludes the edge itself
    auto get_touching_edges = [=](EdgeRef e) {
        std::unordered_set<EdgeRef> touching_edges;
        auto t1 = get_touching_edges_to_vertex(e->halfedge()->vertex());
        touching_edges.insert(t1.begin(), t1.end());
        auto t2 = get_touching_edges_to_vertex(e->halfedge()->twin()->vertex());
        touching_edges.insert(t2.begin(), t2.end());
        touching_edges.erase(e);
        return touching_edges;
    };

    for (size_t iter = 0; iter < num_deletions; ++iter) {
        Edge_Record er = edge_queue.top();
        edge_queue.pop();
        Mat4 q1 = vertex_quadrics[er.edge->halfedge()->vertex()];
        Mat4 q2 = vertex_quadrics[er.edge->halfedge()->twin()->vertex()];
        Mat4 quadric_new = q1 + q2;
        vertex_quadrics.erase(er.edge->halfedge()->vertex());
        vertex_quadrics.erase(er.edge->halfedge()->twin()->vertex());

        auto touching_edges = get_touching_edges(er.edge);
        for (EdgeRef edge : touching_edges) {
            // don't want to add back an uncollapsable edge
            if (edge_records.find(edge) != edge_records.end()) {
                edge_queue.remove(edge_records[edge]);
            }
        }

        auto v_or = collapse_edge_erase(er.edge);
        if (v_or.has_value()) {
            VertexRef v = v_or.value();
            v->pos = er.optimal;
            vertex_quadrics[v] = quadric_new;
            touching_edges = get_touching_edges_to_vertex(v);
        }

        edge_records.erase(er.edge);
        // Add back touching edges
        for (EdgeRef edge : touching_edges) {
            // only add back in records that haven't been before
            if (edge_records.find(edge) != edge_records.end()) {
                edge_records[edge] = {vertex_quadrics, edge};
                edge_queue.insert(edge_records[edge]);
            }
        }
    }

    return true;
}
