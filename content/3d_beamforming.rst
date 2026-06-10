.. _3d-beamforming-chapter:

##############
3D Beamforming
##############

This chapter extends the 2D beamforming/DOA concepts to three-dimensional volumetric arrays. While the mathematical framework we've built already handles 3D element positions, the practical implementation of 3D arrays introduces new challenges: computational complexity, array geometries, visualization of results, and real-world hardware constraints. We'll explore several 3D array geometries, develop beamforming techniques for volumetric arrays, and work through a practical example using a tetrahedral array.

****************************
From 2D Planar to 3D Arrays
****************************

The transition from 2D (planar) to 3D (volumetric) arrays is conceptually simple but practically complex. Recall that the generalized steering vector equation is geometry-agnostic:

.. math::

   s = e^{2j \pi \boldsymbol{p} u / \lambda}

where :math:`\boldsymbol{p}` is the set of element positions (size :code:`Nr` x 3) and :math:`u` is a unit direction vector (size 3x1). This equation works for any array as long as element positions are defined in 3D space—whether the array is planar, volumetric, or even irregular.

In 2D beamforming, all elements occupied a single plane (e.g., the X-Z plane with y=0). Now we allow elements to occupy all three dimensions, enabling us to:

1. **Improve directivity** — More spatial diversity allows sharper beams
2. **Reduce grating lobes** — Better use of 3D space can suppress aliasing
3. **Create arbitrary 3D beam patterns** — Not limited to patterns that respect planar symmetry
4. **Enable 3D DOA estimation** — Localize sources in full 3D space

The price is computational cost: with N elements in 3D, the number of steering vector evaluations during DOA estimation or beam scanning grows significantly. Additionally, visualizing 3D beam patterns is challenging (we'll explore 2D slices and interactive tools).

*********************
3D Array Geometries
*********************

Several canonical 3D array geometries are used in practice:

**Cubic (Rectangular Lattice)**

The simplest extension from a 2D rectangular array is a 3D cubic grid:

.. code-block:: python

 # Cubic array: N x M x L elements
 Nr = 64  # Total elements
 rows, cols, depth = 4, 4, 4  # 4^3 = 64
 d = 0.5 * wavelength  # Half-wavelength spacing

 pos = np.zeros((Nr, 3))
 idx = 0
 for i in range(rows):
     for j in range(cols):
         for k in range(depth):
             pos[idx, 0] = d * i  # x
             pos[idx, 1] = d * j  # y
             pos[idx, 2] = d * k  # z
             idx += 1

A cubic array with half-wavelength spacing has good performance across the entire sphere of directions. However, it requires many elements and is computationally expensive for DOA/beamforming.

**Tetrahedral (Minimal 3D Array)**

A tetrahedron is the minimal 3D array: four elements placed at the vertices of a regular tetrahedron. While small, it provides 3D coverage with minimal hardware:

.. code-block:: python

 # Regular tetrahedron with vertices on a sphere
 import numpy as np

 # Tetrahedron vertices (inscribed in unit sphere)
 theta = np.arccos(1/3)  # ~70.5 degrees

 pos = np.array([
     [0, 0, 1],                    # Top
     [np.sin(theta), 0, -1/3],     # Bottom front
     [np.sin(theta)*np.cos(2*np.pi/3), np.sin(theta)*np.sin(2*np.pi/3), -1/3],  # Bottom left
     [np.sin(theta)*np.cos(4*np.pi/3), np.sin(theta)*np.sin(4*np.pi/3), -1/3],  # Bottom right
 ])

 # Scale to desired wavelength spacing
 scale = 0.5 * wavelength / np.linalg.norm(pos[0] - pos[1])
 pos = pos * scale

Tetrahedra are used in seismic monitoring (4 geophones) and some compact radar systems. The tradeoff is reduced directivity compared to larger 3D arrays.

**Spherical (Uniform Angular Distribution)**

A spherical array approximates equal angular spacing by distributing elements on a sphere's surface:

.. code-block:: python

 # Fibonacci sphere: approximately uniform distribution on a sphere
 def fibonacci_sphere(nr_elements, radius):
     pos = np.zeros((nr_elements, 3))
     golden_angle = np.pi * (3.0 - np.sqrt(5.0))

     for i in range(nr_elements):
         y = 1 - (i / float(nr_elements - 1)) * 2  # y goes from 1 to -1
         radius_at_y = np.sqrt(1 - y * y)

         theta = golden_angle * i
         x = np.cos(theta) * radius_at_y
         z = np.sin(theta) * radius_at_y

         pos[i] = [x, y, z]

     return pos * radius

 pos = fibonacci_sphere(32, 0.5 * wavelength)

Spherical arrays provide omni-directional coverage with roughly equal element spacing. They're expensive (many elements) but achieve excellent directivity.

**Pyramid (Hybrid Approach)**

A pyramid combines a planar rectangular array (2D) with a vertical spine:

.. code-block:: python

 # Rectangular base (4x4 in XY) plus apex
 d = 0.5 * wavelength
 base_rows, base_cols = 4, 4
 apex_height = 2 * d

 pos = []

 # Base (XY plane)
 for i in range(base_rows):
     for j in range(base_cols):
         pos.append([d*i, d*j, 0])

 # Apex
 pos.append([d*(base_rows-1)/2, d*(base_cols-1)/2, apex_height])

 pos = np.array(pos)

Pyramids are practical for systems with strong constraints (e.g., mounted on aircraft where extending in Z is easier than expanding in XY).

---

**Comparing Geometries**

.. list-table::
   :widths: 20 15 15 15 15 20
   :header-rows: 1

   * - Geometry
     - # Elements
     - Directivity
     - Complexity
     - 3D Coverage
     - Use Case
   * - Cubic (4×4×4)
     - 64
     - Excellent
     - O(N²)
     - Excellent
     - Research, large arrays
   * - Tetrahedral
     - 4
     - Poor
     - O(N²)
     - Good
     - Seismic, compact systems
   * - Spherical (32)
     - 32
     - Very Good
     - O(N²)
     - Excellent
     - Omni-directional radar
   * - Pyramid (4×4+1)
     - 17
     - Good
     - O(N²)
     - Good
     - Practical (mounted systems)

---

***************************
Generalized DOA for 3D
***************************

In 2D, we scanned azimuth (:math:`\theta`) and elevation (:math:`\phi`) as a 2D grid. For 3D, the approach is the same: scan the sphere of directions. We can parameterize the unit direction vector using spherical coordinates:

.. code-block:: python

 def get_unit_vector_3d(theta, phi):
     """Convert azimuth and elevation to unit direction vector"""
     return np.array([
         np.sin(theta) * np.cos(phi),  # x component
         np.cos(theta) * np.cos(phi),  # y component
         np.sin(phi)                   # z component
     ]).reshape(-1, 1)

The MUSIC algorithm for 3D is identical to 2D, but the scan grid is now over all :math:`(\theta, \phi)` pairs covering the full sphere (:math:`\theta \in [-\pi, \pi]`, :math:`\phi \in [-\pi/2, \pi/2]`):

.. code-block:: python

 # 3D MUSIC DOA estimation
 R = np.cov(r)  # Covariance matrix from received samples
 w, v = np.linalg.eig(R)
 eig_order = np.argsort(np.abs(w))
 v = v[:, eig_order]

 # Noise subspace (assuming K signals)
 num_signals = 2
 V_noise = v[:, :-num_signals]

 # Scan 3D directions
 theta_scan = np.linspace(-np.pi, np.pi, 180)     # Full azimuth
 phi_scan = np.linspace(-np.pi/2, np.pi/2, 90)    # Full elevation

 music_spectrum = np.zeros((len(theta_scan), len(phi_scan)))

 for i, theta_i in enumerate(theta_scan):
     for j, phi_i in enumerate(phi_scan):
         u = get_unit_vector_3d(theta_i, phi_i)
         s = steering_vector(pos, u)
         metric = 1.0 / np.linalg.norm(V_noise.conj().T @ s)
         music_spectrum[i, j] = np.abs(metric) ** 2

**Computational Cost:** With a 180×90 grid (typical), we evaluate 16,200 steering vectors per DOA estimation. For 64-element arrays, each steering vector is 64 values, and each MUSIC metric involves a matrix operation. This is feasible on modern CPUs but becomes expensive for real-time applications.

**Visualization Challenge:** A 2D heatmap (azimuth vs elevation) is standard, but it's harder to "see" all peaks since the sphere wraps around. You may want to show multiple azimuth slices or use 3D surface plots sparingly (they're slow).

---

*****************************
3D MVDR Beamforming
*****************************

MVDR (Minimum Variance Distortionless Response) in 3D is also straightforward; the weight vector is:

.. math::

   \boldsymbol{w} = \frac{\boldsymbol{R}^{-1} \boldsymbol{s}}{boldsymbol{s}^H \boldsymbol{R}^{-1} \boldsymbol{s}}

where :math:`\boldsymbol{R}` is the spatial covariance matrix and :math:`\boldsymbol{s}` is the steering vector for the 3D direction of interest. The only difference from 2D is that :math:`\boldsymbol{s}` is computed using a 3D unit direction vector.

**Example: Steering and Suppressing Interferers in 3D**

.. code-block:: python

 # Simulate signals from different 3D directions
 theta_signal = np.deg2rad(45)     # Azimuth 45°
 phi_signal = np.deg2rad(30)       # Elevation 30°

 theta_jammer1 = np.deg2rad(-30)
 phi_jammer1 = np.deg2rad(-15)

 theta_jammer2 = np.deg2rad(120)
 phi_jammer2 = np.deg2rad(45)

 # Generate received samples
 N = 10000
 signal_steer = steering_vector(pos, get_unit_vector_3d(theta_signal, phi_signal))
 jammer1_steer = steering_vector(pos, get_unit_vector_3d(theta_jammer1, phi_jammer1))
 jammer2_steer = steering_vector(pos, get_unit_vector_3d(theta_jammer2, phi_jammer2))

 signal_tone = np.exp(2j * np.pi * 0.1 * np.arange(N))
 jammer1_tone = np.exp(2j * np.pi * 0.15 * np.arange(N))
 jammer2_tone = np.exp(2j * np.pi * 0.2 * np.arange(N))

 noise = np.random.randn(len(pos), N) + 1j * np.random.randn(len(pos), N)

 r = (signal_steer @ signal_tone.reshape(1, -1) +
      jammer1_steer @ jammer1_tone.reshape(1, -1) +
      jammer2_steer @ jammer2_tone.reshape(1, -1) +
      noise)

 # Compute MVDR weights pointing at signal
 R = (r @ r.conj().T) / N
 s = steering_vector(pos, get_unit_vector_3d(theta_signal, phi_signal))
 Rinv = np.linalg.pinv(R)
 w_mvdr = (Rinv @ s) / (s.conj().T @ Rinv @ s)

 # Test response towards different directions
 test_angles = [
     (theta_signal, phi_signal, "Target"),
     (theta_jammer1, phi_jammer1, "Jammer 1"),
     (theta_jammer2, phi_jammer2, "Jammer 2"),
 ]

 for theta_t, phi_t, label in test_angles:
     s_test = steering_vector(pos, get_unit_vector_3d(theta_t, phi_t))
     resp = np.abs((w_mvdr.conj().T @ s_test)[0, 0])
     resp_db = 10 * np.log10(resp)
     print(f"{label}: {resp_db:.2f} dB")

Output (example):
```
Target:  0.00 dB
Jammer 1: -15.3 dB
Jammer 2: -18.1 dB
```

The 3D MVDR weights adapt to create nulls in the jammer directions while maintaining unit gain toward the target. Compared to a conventional beamformer, MVDR provides better interference rejection.

---

*******************
Visualization in 3D
*******************

Unlike 2D, which displays naturally as a 2D heatmap, 3D beam patterns require more creativity:

**Option 1: Azimuth Slices (Simplest)**

Show the beam pattern at fixed elevation angles:

.. code-block:: python

 fig, axes = plt.subplots(2, 3, figsize=(15, 10))
 phi_slices = np.deg2rad([-30, -15, 0, 15, 30, 45])

 for idx, phi_val in enumerate(phi_slices):
     ax = axes.flat[idx]
     theta_scan = np.linspace(-np.pi, np.pi, 360)
     power = []

     for theta_i in theta_scan:
         s = steering_vector(pos, get_unit_vector_3d(theta_i, phi_val))
         p = np.abs((w.conj().T @ s)[0, 0]) ** 2
         power.append(p)

     ax.plot(np.rad2deg(theta_scan), 10*np.log10(power))
     ax.set_title(f'Elevation φ = {np.rad2deg(phi_val):.0f}°')
     ax.set_xlabel('Azimuth θ (degrees)')
     ax.set_ylabel('Power (dB)')
     ax.grid()
     ax.set_ylim([-40, 0])

This approach shows 1D slices at multiple elevation angles, making it easy to see how the beam shape changes with elevation.

**Option 2: 3D Surface Plot (Pretty but Slow)**

Visualize the full 3D beam pattern as a surface in spherical coordinates:

.. code-block:: python

 from mpl_toolkits.mplot3d import Axes3D

 theta_scan = np.linspace(-np.pi, np.pi, 100)
 phi_scan = np.linspace(-np.pi/2, np.pi/2, 50)

 THETA, PHI = np.meshgrid(theta_scan, phi_scan)
 POWER = np.zeros_like(THETA)

 for i in range(len(theta_scan)):
     for j in range(len(phi_scan)):
         s = steering_vector(pos, get_unit_vector_3d(THETA[j,i], PHI[j,i]))
         POWER[j, i] = np.abs((w.conj().T @ s)[0, 0]) ** 2

 # Convert to dB and clip for visualization
 POWER_db = 10 * np.log10(POWER + 1e-10)
 POWER_db = np.clip(POWER_db, -40, 0)

 # Convert spherical to Cartesian for 3D plot
 X = POWER_db * np.sin(THETA) * np.cos(PHI)
 Y = POWER_db * np.cos(THETA) * np.cos(PHI)
 Z = POWER_db * np.sin(PHI)

 fig = plt.figure(figsize=(10, 8))
 ax = fig.add_subplot(111, projection='3d')
 ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.9)
 ax.set_xlabel('X')
 ax.set_ylabel('Y')
 ax.set_zlabel('Z')
 plt.show()

**Option 3: Interactive JavaScript Tool (Best UX)**

An interactive web tool (like the 2D array visualizer) allows users to rotate the 3D pattern, hover for values, and adjust parameters dynamically. This is beyond scope for this chapter but highly recommended for practical work.

---

**************************
Practical 3D Example: Sonar
**************************

Underwater sonar systems commonly use 3D volumetric arrays. Consider a hemispherical sonar receiver with elements arranged in concentric rings:

.. code-block:: python

 # Hemispherical sonar array (32 elements)
 # Arranged in 4 rings at different depths
 nr_rings = 4
 elements_per_ring = 8

 radius = 0.5 * wavelength
 depths = [0, radius*0.5, radius*1.0, radius*1.5]  # Ring depths (z)

 pos = []
 for ring_idx, z in enumerate(depths):
     ring_radius = radius * np.sqrt(1 - (z/radius)**2)  # Scale radius for hemisphere
     for elem_idx in range(elements_per_ring):
         angle = 2*np.pi*elem_idx/elements_per_ring
         x = ring_radius * np.cos(angle)
         y = ring_radius * np.sin(angle)
         pos.append([x, y, z])

 pos = np.array(pos)

 # Plot the geometry
 from mpl_toolkits.mplot3d import Axes3D
 fig = plt.figure()
 ax = fig.add_subplot(111, projection='3d')
 ax.scatter(pos[:,0], pos[:,1], pos[:,2], c='blue', marker='o', s=100)
 ax.set_xlabel('X (m)')
 ax.set_ylabel('Y (m)')
 ax.set_zlabel('Z (m)')
 ax.set_title('Hemispherical Sonar Array')
 plt.show()

**Detecting underwater targets:**

Suppose a target is at azimuth 60° and elevation -20° (below boresight):

.. code-block:: python

 theta_target = np.deg2rad(60)
 phi_target = np.deg2rad(-20)

 # Conventional beamformer
 s_target = steering_vector(pos, get_unit_vector_3d(theta_target, phi_target))
 w_conv = s_target / np.linalg.norm(s_target)

 # MVDR beamformer (with sample received signal)
 N_samples = 5000
 signal = np.exp(2j * np.pi * 0.05 * np.arange(N_samples))  # 50 kHz IF signal
 noise = 0.1 * (np.random.randn(len(pos), N_samples) + 1j*np.random.randn(len(pos), N_samples))
 r = s_target @ signal.reshape(1, -1) + noise

 R = (r @ r.conj().T) / N_samples
 Rinv = np.linalg.pinv(R)
 w_mvdr = (Rinv @ s_target) / (s_target.conj().T @ Rinv @ s_target)

 # Scan and detect
 theta_scan = np.linspace(-np.pi, np.pi, 72)
 phi_scan = np.linspace(-np.pi/2, np.pi/2, 36)

 detection_map = np.zeros((len(theta_scan), len(phi_scan)))

 for i, theta_i in enumerate(theta_scan):
     for j, phi_i in enumerate(phi_scan):
         s_test = steering_vector(pos, get_unit_vector_3d(theta_i, phi_i))
         power_conv = np.abs((w_conv.conj().T @ s_test)[0, 0]) ** 2
         power_mvdr = np.abs((w_mvdr.conj().T @ s_test)[0, 0]) ** 2
         detection_map[i, j] = 10*np.log10(power_mvdr)

 # Find peak
 peak_idx = np.unravel_index(np.argmax(detection_map), detection_map.shape)
 peak_theta = theta_scan[peak_idx[0]]
 peak_phi = phi_scan[peak_idx[1]]

 print(f"Detected target: θ={np.rad2deg(peak_theta):.1f}°, φ={np.rad2deg(peak_phi):.1f}°")
 print(f"True target:    θ={np.rad2deg(theta_target):.1f}°, φ={np.rad2deg(phi_target):.1f}°")

---

***************************
Challenges and Trade-offs
***************************

**Computational Complexity**

The steering vector computation remains O(Nr) per evaluation, but DOA scanning is O(Nθ × Nφ × Nr²) for MUSIC (due to eigendecomposition). For a 64-element 3D array with 180×90 direction grid, this is hundreds of millions of operations. Practical systems often:

- Pre-compute steering vectors in lookup tables
- Use FFT-based beamforming for fast scanning
- Reduce resolution dynamically (coarse→fine search)
- Parallelize across GPU/FPGA

**Array Calibration**

3D arrays are harder to calibrate than 2D (or 1D). Phase and amplitude errors compound across all three dimensions. Calibration typically requires:

1. Known reference source (ideally omnidirectional)
2. Measurement from each element
3. Per-element phase/amplitude correction lookup table
4. Validation with secondary sources

**Element Mutual Coupling**

In compact 3D arrays (e.g., tetrahedra), elements are close together, leading to strong mutual coupling between antennas. This degrades the array's effective response and must be compensated by advanced calibration or pre-processing.

**Practical Beam Shape**

Real 3D beamformer beams are not perfectly spherical; they exhibit directional dependence (the beam is sharper in some directions, broader in others). This is due to:

- Non-uniform element spacing relative to wavelength at different angles
- Element patterns (not truly isotropic)
- Aliasing artifacts

---

**Summary**

3D beamforming extends the framework from 2D with minimal mathematical change but significant practical implications:

- **Array Design:** Choose geometry based on aperture size, coverage requirements, and computational budget
- **DOA Estimation:** MUSIC works in 3D but costs more; scan all azimuth/elevation pairs
- **Adaptive Beamforming:** MVDR suppresses 3D interference but requires stable covariance matrix estimation
- **Visualization:** Use azimuth slices, 3D plots, or interactive tools depending on application
- **Implementation:** Real-time 3D beamforming requires careful optimization (lookup tables, parallel processing)

The mathematical tools remain the same; success lies in thoughtful array design and efficient implementation.

---

**References**

- PySDR Beamforming & DOA chapters (1D, 2D foundations)
- "Phased Array Antenna Handbook" (second edition), Mailloux, R. J.
- "Sonar Array Processing," Baggeroer, A. et al., Proceedings of IEEE, 1992
- MATLAB documentation on 3D array processing and phased array design toolbox
