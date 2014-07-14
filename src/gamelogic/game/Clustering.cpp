/*
===========================================================================

Copyright 2014 Unvanquished Developers

This file is part of Unvanquished.

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "g_local.h"

namespace Clustering {
	/**
	 * @brief A point in euclidean space.
	 * @todo Use engine's vector class once available.
	 */
	template <int Dim>
	class Point {
		public:
			float coords[Dim];

			Point() = default;

			Point(const float vec[Dim]) {
				for (int i = 0; i < Dim; i++) coords[i] = vec[i];
			}

			Point(const Point& other) {
				for (int i = 0; i < Dim; i++) coords[i] = other.coords[i];
			}

			void Clear() {
				for (int i = 0; i < Dim; i++) coords[i] = 0;
			}

			float& operator[](int i) {
				return coords[i];
			}

			const float& operator[](int i) const {
				return coords[i];
			}

			Point<Dim> operator+(const Point<Dim>& other) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] + other[i];
				return result;
			}

			Point<Dim> operator-(const Point<Dim>& other) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] - other[i];
				return result;
			}

			Point<Dim> operator*(float value) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] * value;
				return result;
			}

			Point<Dim> operator/(float value) const {
				Point<Dim> result;
				for (int i = 0; i < Dim; i++) result[i] = coords[i] / value;
				return result;
			}

			void operator+=(const Point<Dim>& other) {
				for (int i = 0; i < Dim; i++) coords[i] += other[i];
			}

			void operator-=(const Point<Dim>& other) {
				for (int i = 0; i < Dim; i++) coords[i] -= other[i];
			}

			void operator*=(float value) {
				for (int i = 0; i < Dim; i++) coords[i] *= value;
			}

			void operator/=(float value) {
				for (int i = 0; i < Dim; i++) coords[i] /= value;
			}

			bool operator==(const Point<Dim>& other) const {
				for (int i = 0; i < Dim; i++) if (coords[i] != other[i]) return false;
				return true;
			}

			float Dot(const Point& other) const {
				float product = 0;
				for (int i = 0; i < Dim; i++) product += coords[i] * other[i];
				return product;
			}

			float Distance(const Point& other) const {
				float distanceSquared = 0;
				for (int i = 0; i < Dim; i++) {
					float delta = coords[i] - other[i];
					distanceSquared += delta * delta;
				}
				return sqrtf(distanceSquared);
			}
	};

	/**
	 * @brief A cluster of objects located in euclidean space.
	 */
	template <typename Data, int Dim>
	class EuclideanCluster {
		public:
			typedef Point<Dim>                                                    point_type;
			typedef std::pair<Data, point_type>                                   record_type;
			typedef typename std::unordered_map<Data, point_type>::const_iterator iter_type;

			EuclideanCluster() = default;

			/**
			 * @brief Updates the cluster while keeping track of metadata.
			 */
			void Update(const Data& data, const point_type& location) {
				records.insert(std::make_pair(data, location));
				dirty = true;
			}

			/**
			 * @brief Removes data from the cluster while keeping track of metadata.
			 */
			void Remove(const Data& data) {
				records.erase(data);
				dirty = true;
			}

			/**
			 * @return The euclidean center of the cluster.
			 */
			const point_type& GetCenter() {
				if (dirty) UpdateMetadata();
				return center;
			}

			Data GetMeanObject() {
				if (dirty) UpdateMetadata();
				return meanObject;
			}

			/**
			 * @return The average distance of objects from the center.
			 */
			float GetAverageDistance() {
				if (dirty) UpdateMetadata();
				return averageDistance;
			}

			/**
			 * @return The standard deviation from the average distance to the center.
			 */
			float GetStandardDeviation() {
				if (dirty) UpdateMetadata();
				return standardDeviation;
			}

			iter_type begin() const noexcept {
				return records.begin();
			}

			iter_type end() const noexcept {
				return records.end();
			}

			iter_type find(const Data& key) const {
				return records.find(key);
			}

			size_t size() const {
				return records.size();
			}

		private:
			/**
			 * @brief Calculates cluster metadata.
			 */
			void UpdateMetadata() {
				dirty = false;

				center.Clear();
				meanObject        = nullptr;
				averageDistance   = 0;
				standardDeviation = 0;

				int size = records.size();
				if (size == 0) return;

				float smallestDistance = FLT_MAX;

				// Find center
				for (auto& record : records) {
					center += record.second;
				}
				center /= size;

				// Find average distance and mean object
				for (auto& record : records) {
					point_type& location = record.second;
					float distance = location.Distance(center);
					averageDistance += distance;
					if (distance < smallestDistance) {
						smallestDistance = distance;
						meanObject       = record.first;
					}
				}
				averageDistance /= size;

				// Find standard deviation
				for (auto& record : records) {
					float deviation = averageDistance - record.second.Distance(center);
					standardDeviation += deviation * deviation;
				}
				standardDeviation = sqrtf(standardDeviation / size);
			}

			/** Maps data objects to their location. */
			std::unordered_map<Data, point_type> records;

			/** The cluster's center point. */
			point_type center;

			/** The object closest to the center. */
			Data meanObject;

			/** The average distance of objects from the cluster's center. */
			float averageDistance;

			/** The standard deviation from the average distance to the center. */
			float standardDeviation;

			/** Whether metadata needs to be recalculated. */
			bool dirty;
	};

	/**
	 * @brief A self-organizing container of clusters of objects located in euclidean space.
	 *
	 * The clustering algorihm is based on Kruskal's algorithm for minimum spanning trees.
	 * In the minimum spanning tree of all edges that pass the optional visibility check, delete the
	 * edges that are longer than the average plus the standard deviation multiplied by a "laxity"
	 * factor. The remaining trees span the clusters.
	 */
	template <typename Data, int Dim>
	class EuclideanClustering {
		public:
			typedef EuclideanCluster<Data, Dim>                      cluster_type;
			typedef typename EuclideanCluster<Data, Dim>::point_type point_type;
			typedef Data                                             vertex_type;
			typedef std::pair<Data, point_type>                      vertex_record_type;
			typedef std::pair<Data, Data>                            edge_type;
			typedef std::pair<float, edge_type>                      edge_record_type;
			typedef typename std::vector<cluster_type>::iterator     iter_type;

			/**
			 * @param laxity Factor that scales the allowed deviation from the average edge length.
			 * @param edgeVisCallback Relation that decides whether an edge should be considered.
			 */
			EuclideanClustering(float laxity = 1.0,
			                    std::function<bool(Data, Data)> edgeVisCallback = nullptr)
			    : laxity(laxity), edgeVisCallback(edgeVisCallback)
			{}

			/**
			 * @brief Adds or updates the location of objects.
			 */
			void Update(const Data& data, const Point<Dim>& location) {
				// TODO: Check if the location has actually changed.

				// Remove the object first.
				Remove(data);

				// Iterate over all other objects and save the distance.
				for (const vertex_record_type& record : records) {
					if (edgeVisCallback == nullptr || edgeVisCallback(data, record.first)) {
						float distance = location.Distance(record.second);
						edges.insert(std::make_pair(distance, edge_type(data, record.first)));
					}
				}

				// The object is now known.
				records.insert(std::make_pair(data, location));

				// Rebuild MST and clusters on next read access.
				dirtyMST = true;
			}

			/**
			 * @brief Removes objects.
			 */
			void Remove(const Data& data) {
				// Check if we even know the object in O(n) to avoid unnecessary O(m) iteration.
				if (records.find(data) == records.end()) return;

				// Delete all edges that involve the object.
				for (auto edge = edges.begin(); edge != edges.end(); ) {
				    if ((*edge).second.first == data || (*edge).second.second == data) {
						edge = edges.erase(edge);
					} else {
						edge++;
					}
				}

				// Forget about the object.
				records.erase(data);

				// Rebuild MST and clusters on next read access.
				dirtyMST = true;
			}

			void Clear() {
				records.clear();
				dirtyMST = true;
			}

			/**
			 * @brief Set laxity, a factor that scales the allowed deviation from the average edge
			 *        length when splitting the minimum spanning tree into cluster spanning trees.
			 */
			void SetLaxity(float laxity = 1.0) {
				this->laxity = laxity;
				dirtyClusters = true;
			}

			iter_type begin() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.begin();
			}

			iter_type end() {
				if (dirtyMST || dirtyClusters) GenerateClusters();
				return clusters.end();
			}

		private:
			/**
			 * @brief Finds the minimum spanning tree in the graph defined by edges, where edge
			 *        weight is the euclidean distance of the data object's location.
			 *
			 * Uses Kruskal's algorithm.
			 */
			void FindMST() {
				// Clear an existing MST.
				mstEdges.clear();
				mstAverageDistance   = 0;
				mstStandardDeviation = 0;

				// Track connected components for circle prevention.
				DisjointSets<Data> components = DisjointSets<Data>();

				// The edges are implicitely sorted by distance, iterate in ascending order.
				for (const edge_record_type& edgeRecord : edges) {
					float distance        = edgeRecord.first;
					const edge_type& edge = edgeRecord.second;

					// Stop if spanning tree is complete.
					if ((mstEdges.size() + 1) == records.size()) break;

					// Get component representatives, if available.
					Data firstVertexRepr  = components.Find(edge.first);
					Data secondVertexRepr = components.Find(edge.second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSetFast(edge.first);
					}
					if (secondVertexRepr == nullptr) {
						secondVertexRepr = components.MakeSetFast(edge.second);
					}

					// Don't create circles.
					if (firstVertexRepr == secondVertexRepr) continue;

					// Mark components as connected.
					components.Link(firstVertexRepr, secondVertexRepr);

					// Add the edge to the MST.
					mstEdges.insert(std::make_pair(distance, edge));

					// Add distance to average.
					mstAverageDistance += distance;
				}

				// Save metadata.
				int numMstEdges = mstEdges.size();
				if (numMstEdges != 0) {
					// Average distances.
					mstAverageDistance /= numMstEdges;

					// Find standard deviation.
					for (const edge_record_type& edgeRecord : mstEdges) {
						float deviation = mstAverageDistance - edgeRecord.first;
						mstStandardDeviation += deviation * deviation;
					}
					mstStandardDeviation = sqrtf(mstStandardDeviation / numMstEdges);
				}

				dirtyMST = false;
			}

			/**
			 * @brief Generates the clusters.
			 *
			 * In the minimum spanning tree, delete the edges that are longer than the average plus
			 * the standard deviation multiplied by a "laxity" factor. The remaining trees span the
			 * clusters.
			 */
			void GenerateClusters() {
				if (dirtyMST) {
					FindMST();
				}

				// Clear an existing clustering.
				forestEdges.clear();
				clusters.clear();

				// Split the MST into several trees by keeping only the edges that have a length up
				// to a threshold.
				float edgeLengthThreshold = mstAverageDistance + mstStandardDeviation * laxity;
				for (const edge_record_type& edge : mstEdges) {
					if (edge.first <= edgeLengthThreshold) {
						forestEdges.insert(edge);
					} else {
						// Edges are implicitly sorted by distance, so we can stop here.
						break;
					}
				}

				// Retreive the connected components, excluding isolated vertices.
				DisjointSets<Data> components;
				for (const edge_record_type& edgeRecord : forestEdges) {
					const edge_type& edge = edgeRecord.second;

					// Get component representatives, if available.
					Data firstVertexRepr = components.Find(edge.first);
					Data secndVertexRepr = components.Find(edge.second);

					// Otheriwse add vertices to new components.
					if (firstVertexRepr == nullptr) {
						firstVertexRepr = components.MakeSetFast(edge.first);
					}
					if (secndVertexRepr == nullptr) {
						secndVertexRepr = components.MakeSetFast(edge.second);
					}

					// Mark components as connected.
					components.Link(firstVertexRepr, secndVertexRepr);
				}

				// Keep track of non-clustered vertices.
				std::unordered_set<Data> remainingVertices;
				for (const vertex_record_type& record : records) {
					remainingVertices.insert(record.first);
				}

				// Build a cluster for each connected component, excluding isolated vertices.
				for (auto& component : components) {
					cluster_type newCluster;
					for (const Data& vertex : component.second) {
						newCluster.Update(vertex, records[vertex]);
						remainingVertices.erase(vertex);
					}
					clusters.push_back(newCluster);
				}

				// The remaining vertices are isolated, they go in a cluster of their own.
				for (const Data& vertex : remainingVertices) {
					cluster_type newCluster;
					newCluster.Update(vertex, records[vertex]);
					clusters.push_back(newCluster);
				}

				dirtyClusters = false;
			}

			/** The generated clusters. */
			std::vector<cluster_type> clusters;

			/** Maps data objects to their location. */
			std::unordered_map<Data, point_type> records;

			/**  The edges of a non-reflexive graph of the data objects, sorted by distance. */
			std::multimap<float, edge_type> edges;

			/** The edges of the minimum spanning tree in the graph defined by edges, sorted by
			 *  distance. Is a subset of edges. */
			std::multimap<float, edge_type> mstEdges;

			/** The edges of a forest of which each connected component spans a cluster. Is a subset
			 *  of mstEdges. */
			std::multimap<float, edge_type> forestEdges;

			/** The average edge length in the minimum spanning tree. */
			float mstAverageDistance;

			/** The standard deviation of the edge length in the minimum spanning tree. */
			float mstStandardDeviation;

			/** Whether clusters need to be rebuilt on read acces. Does not imply regeneration of
			 *  the minimum spanning tree. */
			bool dirtyClusters;

			/** Whether the minimum spanning tree needs to be rebuild on read access. Implies
			 *  regeneration of clusters. */
			bool dirtyMST;

			/** A factor that scales the allowed deviation from the average edge length when
			 *  splitting the minimum spanning tree into cluster spanning trees. */
			float laxity;

			/** A callback relation that decides whether an edge should be part of edges.
			 *  Needs to be symmetric as edges are bidirectional. */
			std::function<bool(Data, Data)> edgeVisCallback;
	};

	/**
	 * @brief A clustering of entities in the world.
	 */
	class EntityClustering : public EuclideanClustering<gentity_t*, 3> {
		public:
			typedef Clustering::EuclideanClustering<gentity_t*, 3> super;

			EntityClustering(float laxity = 1.0,
							 std::function<bool(gentity_t*, gentity_t*)> edgeVisCallback = nullptr)
				: super(laxity, edgeVisCallback)
			{}

			void Update(gentity_t *ent) {
				super::Update(ent, point_type(ent->s.origin));
			}

			/**
			 * @brief An edge visibility check that checks for PVS visibility.
			 */
			static bool edgeVisPVS(gentity_t *a, gentity_t *b) {
				return trap_InPVSIgnorePortals(a->s.origin, b->s.origin);
			}
	};
}

/**
 * @brief Uses EntityClusterings to keep track of the bases of both teams.
 */
namespace BaseClustering {
	using namespace Clustering;

	static std::map<team_t, EntityClustering>               bases;
	static std::map<team_t, std::unordered_set<gentity_t*>> beacons;

	/**
	 * @brief Called after calls to Update and Remove.
	 */
	static void PostChangeHook(team_t team) {
		std::unordered_set<gentity_t*> &teamBeacons = beacons[team];
		std::unordered_set<gentity_t*> newBeacons;

		// Add a beacon for every cluster
		for (EntityClustering::cluster_type& cluster : bases[team]) {
			const EntityClustering::point_type &center = cluster.GetCenter();
			gentity_t *mean                            = cluster.GetMeanObject();
			float averageDistance                      = cluster.GetAverageDistance();
			float standardDeviation                    = cluster.GetStandardDeviation();

			// Find out if it's an outpost or main base and see if any of the inner structures is
			// tagged by the enemy
			team_t taggedByEnemy = TEAM_NONE;
			bool   mainBase      = false;
			for (const EntityClustering::cluster_type::record_type& record : cluster) {
				gentity_t* ent = record.first;
				float distance = record.second.Distance(center);

				// TODO: Use G_IsMainStructure when merged.
				if (ent->s.modelindex == BA_A_OVERMIND || ent->s.modelindex == BA_H_REACTOR) {
					mainBase = true;
				}

				if (ent->taggedByEnemy && distance <= averageDistance + standardDeviation) {
					taggedByEnemy = ent->taggedByEnemy;
				}
			}
			int eFlags = mainBase ? 0 : EF_BC_BASE_OUTPOST;

			// Trace from mean buildable's origin towards cluster center, so that the beacon does
			// not spawn inside a wall. Then use MoveTowardsRoom on the trace results.
			trace_t tr;
			trap_Trace(&tr, mean->s.origin, NULL, NULL, center.coords, 0, MASK_SOLID);
			Beacon::MoveTowardsRoom(tr.endpos, NULL);

			gentity_t *beacon;

			// If a fitting beacon close to the target location already exists, move it silently,
			// otherwise add a new one.
			if ((beacon = Beacon::FindSimilar(center.coords, BCT_BASE, 0, team, 0,
			                                  averageDistance, eFlags))) {
				VectorCopy(tr.endpos, beacon->s.origin);
			} else {
				beacon = Beacon::New(tr.endpos, BCT_BASE, 0, team, ENTITYNUM_NONE);
				beacon->s.eFlags |= eFlags;
				Beacon::Propagate(beacon);
			}
			newBeacons.insert(beacon);

			// Add a second beacon for the enemy team if they tagged the base.
			if (taggedByEnemy) {
				eFlags |= EF_BC_BASE_ENEMY;

				if ((beacon = Beacon::FindSimilar(center.coords, BCT_BASE, 0, taggedByEnemy, 0,
				                                  averageDistance, eFlags))) {
					VectorCopy(tr.endpos, beacon->s.origin);
				} else {
					beacon = Beacon::New(tr.endpos, BCT_BASE, 0, taggedByEnemy, ENTITYNUM_NONE);
					beacon->s.eFlags |= eFlags;
					Beacon::Propagate(beacon);
				}

				newBeacons.insert(beacon);
			}
		}

		// Delete all orphaned base beacons.
		for (gentity_t *beacon : teamBeacons) {
			if (newBeacons.find(beacon) == newBeacons.end()) {
				// TODO: Use verbose beacon unlinking as this base actually disappeared
				Beacon::Delete(beacon);
			}
		}
		teamBeacons.swap(newBeacons);
	}

	/**
	 * @brief Resets the clusterings.
	 */
	void Init() {
		// Reset clusterings and beacon lists for both teams
		for (int teamNum = TEAM_NONE + 1; teamNum < NUM_TEAMS; teamNum++) {
			team_t team = (team_t)teamNum;

			// Reset clusterings
			std::map<team_t, EntityClustering>::iterator teamBases;
			if ((teamBases = bases.find(team)) != bases.end()) {
				teamBases->second.Clear();
			} else {
				bases.insert(std::make_pair(
					team, EntityClustering(2.5, EntityClustering::edgeVisPVS)));
			}

			// Reset beacon lists
			std::map<team_t, std::unordered_set<gentity_t*>>::iterator teamBaseBeacons;
			if ((teamBaseBeacons = beacons.find(team)) != beacons.end()) {
				teamBaseBeacons->second.clear();
			} else {
				beacons.insert(std::make_pair(team, std::unordered_set<gentity_t*>()));
			}
		}
	}

	/**
	 * @brief Adds a buildable to the clusters or updates its location.
	 */
	void Update(gentity_t *ent) {
		EntityClustering& clusters = bases[ent->buildableTeam];
		clusters.Update(ent);
		PostChangeHook(ent->buildableTeam);
	}

	/**
	 * @brief Removes a buildable from the clusters.
	 */
	void Remove(gentity_t *ent) {
		EntityClustering& clusters = bases[ent->buildableTeam];
		clusters.Remove(ent);
		PostChangeHook(ent->buildableTeam);
	}

	/**
	 * @brief Updates base beacons for a team as if structures changed.
	 */
	void Touch(team_t team) {
		PostChangeHook(team);
	}

	/**
	 * @brief Prints debugging information and spawns pointer beacons for every cluster.
	 */
	void Debug() {
		for (int teamNum = TEAM_NONE + 1; teamNum < NUM_TEAMS; teamNum++) {
			team_t team = (team_t)teamNum;
			int clusterNum = 1;

			for (EntityClustering::cluster_type& cluster : bases[team]) {
				const EntityClustering::point_type& center = cluster.GetCenter();

				Com_Printf("%s base #%d (%lu buildings): Center: %.0f %.0f %.0f. "
						   "Avg. distance: %.0f. Standard deviation: %.0f.\n",
						   BG_TeamName(team), clusterNum, cluster.size(),
						   center[0], center[1], center[2],
						   cluster.GetAverageDistance(), cluster.GetStandardDeviation());

				Beacon::Propagate(Beacon::New(cluster.GetCenter().coords, BCT_POINTER, 0,
											  team, ENTITYNUM_NONE));

				clusterNum++;
			}
		}
	}
}