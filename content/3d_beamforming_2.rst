.. _3d-beamforming-fundamentals:

#############################
3D Beamforming: Fundamentals
#############################

This chapter provides a rigorous mathematical treatment of three-dimensional phased array beamforming. Rather than focusing on implementation, we develop the theoretical foundations: the physics of wave propagation, the mathematics of array signal processing, and the fundamental limits of beamforming performance. This material is essential for understanding why certain beamformers work, their limitations, and their optimality properties.

***********************
Wave Propagation Basics
***********************

**Plane Wave Assumption**

Consider a narrowband signal emanating from a distant source. In the far field (distance :math:`R \gg D^2/\lambda`, where :math:`D` is array dimension), the wavefront arriving at the array is approximately planar. A plane wave propagating in direction :math:`\mathbf{u}` (unit vector) can be written as:

.. math::

   s(\mathbf{r}, t) = A e^{j(2\pi f_0 t - 2\pi \mathbf{u} \cdot \mathbf{r} / \lambda + \phi_0)}

where:

- :math:`\mathbf{r}` is the position vector (x, y, z)
- :math:`f_0` is the carrier frequency
- :math:`\lambda = c/f_0` is the wavelength
- :math:`\phi_0` is an arbitrary phase offset
- :math:`A` is amplitude

**Phase Relationships**

The key quantity in beamforming is the **phase difference** between elements. Consider two receivers at positions :math:`\mathbf{r}_1` and :math:`\mathbf{r}_2` receiving the same plane wave:

.. math::

   \Delta \phi = 2\pi (\mathbf{u} \cdot \mathbf{r}_2 - \mathbf{u} \cdot \mathbf{r}_1) / \lambda = 2\pi \mathbf{u} \cdot (\mathbf{r}_2 - \mathbf{r}_1) / \lambda

The phase shift depends on:
1. The propagation direction :math:`\mathbf{u}`
2. The baseline vector :math:`\mathbf{r}_2 - \mathbf{r}_1`
3. The wavelength :math:`\lambda`

**Generalization to N Elements**

For an array of :math:`N` elements at positions :math:`\{\mathbf{r}_1, \ldots, \mathbf{r}_N\}`, the received signal at element :math:`n` is:

.. math::

   r_n(t) = A e^{j(2\pi f_0 t - 2\pi \mathbf{u} \cdot \mathbf{r}_n / \lambda + \phi_0)} + \eta_n(t)

where :math:`\eta_n(t)` is noise at element :math:`n`. Factoring out the common phase :math:`e^{j 2\pi f_0 t}`:

.. math::

   r_n(t) = A e^{-j 2\pi \mathbf{u} \cdot \mathbf{r}_n / \lambda} e^{j \phi_0} + \eta_n(t)

**Steering Vector Definition**

Define the **steering vector** as the vector of complex phase shifts across the array:

.. math::

   \mathbf{s}(\mathbf{u}) = \begin{bmatrix} e^{-j 2\pi \mathbf{u} \cdot \mathbf{r}_1 / \lambda} \\ e^{-j 2\pi \mathbf{u} \cdot \mathbf{r}_2 / \lambda} \\ \vdots \\ e^{-j 2\pi \mathbf{u} \cdot \mathbf{r}_N / \lambda} \end{bmatrix} = \begin{bmatrix} e^{j \phi_1(\mathbf{u})} \\ e^{j \phi_2(\mathbf{u})} \\ \vdots \\ e^{j \phi_N(\mathbf{u})} \end{bmatrix}

where :math:`\phi_n(\mathbf{u}) = -2\pi \mathbf{u} \cdot \mathbf{r}_n / \lambda` is the phase at element :math:`n` for direction :math:`\mathbf{u}`.

**Compact Matrix Form**

The received signal from a single source with steering vector :math:`\mathbf{s}(\mathbf{u})` and signal complex amplitude :math:`a(t)` is:

.. math::

   \mathbf{r}(t) = a(t) \mathbf{s}(\mathbf{u}) + \boldsymbol{\eta}(t)

where :math:`\boldsymbol{\eta}(t)` is the noise vector. For narrowband signals, :math:`a(t)` is approximately constant over the array response time and can be factored out.

---

**************************
Vector Space Foundations
**************************

**Signal as Vector in Hilbert Space**

The received array data :math:`\mathbf{r}(t) \in \mathbb{C}^N` lives in a complex Hilbert space :math:`\mathcal{H} = \mathbb{C}^N`. Key properties:

1. **Inner Product:** :math:`\langle \mathbf{a}, \mathbf{b} \rangle = \mathbf{a}^H \mathbf{b}` (conjugate transpose of first vector times second)
2. **Norm:** :math:`\|\mathbf{a}\| = \sqrt{\langle \mathbf{a}, \mathbf{a} \rangle} = \sqrt{\mathbf{a}^H \mathbf{a}}`
3. **Orthogonality:** Vectors are orthogonal if :math:`\langle \mathbf{a}, \mathbf{b} \rangle = 0`

**Steering Vector Properties**

The steering vector has important mathematical properties:

1. **Normalization:** :math:`\|\mathbf{s}(\mathbf{u})\|^2 = \mathbf{s}^H(\mathbf{u}) \mathbf{s}(\mathbf{u}) = N` (sum of :math:`N` unit-magnitude complex numbers)

2. **Unitarity:** The steering vectors from different directions are generally **not orthogonal**, but their inner products encode directional information:

.. math::

   \mathbf{s}^H(\mathbf{u}_1) \mathbf{s}(\mathbf{u}_2) = \sum_{n=1}^{N} e^{j 2\pi (\mathbf{u}_2 - \mathbf{u}_1) \cdot \mathbf{r}_n / \lambda}

   For :math:`\mathbf{u}_1 = \mathbf{u}_2`: result is :math:`N`
   For :math:`\mathbf{u}_1 \neq \mathbf{u}_2`: result has magnitude < :math:`N` (depends on angular separation and array geometry)

3. **Manifold Structure:** The set of all steering vectors :math:`\{\mathbf{s}(\mathbf{u}) : \mathbf{u} \in S^2\}` forms the **array manifold**—a 2D surface in :math:`\mathbb{C}^N`. For typical arrays with :math:`N \gg 2`, the array manifold is a low-dimensional, curved surface embedded in high-dimensional complex space.

---

*****************************
Spatial Covariance Matrix
*****************************

**Definition and Interpretation**

Given time samples :math:`\mathbf{r}(t_1), \ldots, \mathbf{r}(t_M)`, the spatial covariance matrix is:

.. math::

   \mathbf{R} = E[\mathbf{r}(t) \mathbf{r}^H(t)]

In practice, estimated as:

.. math::

   \hat{\mathbf{R}} = \frac{1}{M} \sum_{m=1}^{M} \mathbf{r}(t_m) \mathbf{r}^H(t_m)

Dimensions: :math:`\mathbf{R} \in \mathbb{C}^{N \times N}`, Hermitian positive semi-definite.

**Physical Interpretation**

Each element :math:`[\mathbf{R}]_{ij} = E[r_i(t) r_j^*(t)]` represents the correlation between elements :math:`i` and :math:`j`. This encodes:

- **Diagonal elements** :math:`[\mathbf{R}]_{ii}` — Power at element :math:`i`
- **Off-diagonal magnitude** :math:`|[\mathbf{R}]_{ij}|` — Coherence between elements
- **Off-diagonal phase** :math:`\arg([\mathbf{R}]_{ij})` — Phase relationship between elements

**Eigendecomposition**

Since :math:`\mathbf{R}` is Hermitian positive semi-definite, it has eigen-decomposition:

.. math::

   \mathbf{R} = \sum_{k=1}^{N} \lambda_k \mathbf{v}_k \mathbf{v}_k^H

where :math:`\lambda_1 \geq \lambda_2 \geq \cdots \geq \lambda_N \geq 0` are eigenvalues and :math:`\mathbf{v}_1, \ldots, \mathbf{v}_N` are orthonormal eigenvectors.

**Signal + Noise Model**

Suppose the received signal contains :math:`K` incoherent plane waves plus white noise:

.. math::

   \mathbf{r}(t) = \sum_{k=1}^{K} a_k(t) \mathbf{s}(\mathbf{u}_k) + \boldsymbol{\eta}(t)

Then the covariance matrix can be decomposed:

.. math::

   \mathbf{R} = \mathbf{R}_s + \mathbf{R}_\eta

where:

.. math::

   \mathbf{R}_s = \sum_{k=1}^{K} P_k \mathbf{s}(\mathbf{u}_k) \mathbf{s}^H(\mathbf{u}_k)

   \mathbf{R}_\eta = \sigma_\eta^2 \mathbf{I}

Here :math:`P_k = E[|a_k(t)|^2]` is the power of signal :math:`k` and :math:`\sigma_\eta^2` is the noise power.

**Eigenvalue Interpretation**

The eigendecomposition of :math:`\mathbf{R} = \mathbf{R}_s + \sigma_\eta^2 \mathbf{I}` yields:

- **K largest eigenvalues** correspond to the signal subspace. Each :math:`\lambda_k \approx P_k + \sigma_\eta^2`
- **N-K smallest eigenvalues** correspond to the noise subspace: :math:`\lambda_{k+1} \approx \cdots \approx \lambda_N \approx \sigma_\eta^2`

The eigenvector corresponding to eigenvalue :math:`\lambda_k` spans a direction in the signal+noise space. Crucially, the **noise subspace** eigenvectors are **orthogonal to all steering vectors** :math:`\mathbf{s}(\mathbf{u}_k)` that correspond to actual signal sources.

---

****************************
Conventional Beamforming
****************************

**Beamformer Definition**

A beamformer is a linear spatial filter characterized by a weight vector :math:`\mathbf{w} \in \mathbb{C}^N`:

.. math::

   y(t) = \mathbf{w}^H \mathbf{r}(t)

The output :math:`y(t)` is a scalar time series. The beamformer combines the :math:`N` element signals with complex weights to produce a scalar output.

**Gain in Direction**

Suppose the input is a plane wave in direction :math:`\mathbf{u}_0` with unit amplitude:

.. math::

   \mathbf{r}(t) = e^{j 2\pi f_0 t} \mathbf{s}(\mathbf{u}_0)

Then:

.. math::

   y(t) = e^{j 2\pi f_0 t} \mathbf{w}^H \mathbf{s}(\mathbf{u}_0)

The frequency-domain response is:

.. math::

   H(\mathbf{u}_0) = \mathbf{w}^H \mathbf{s}(\mathbf{u}_0)

The **power gain** (or **beam pattern**) is:

.. math::

   G(\mathbf{u}) = |H(\mathbf{u})|^2 = |\mathbf{w}^H \mathbf{s}(\mathbf{u})|^2 = \mathbf{w}^H \mathbf{s}(\mathbf{u}) \mathbf{s}^H(\mathbf{u}) \mathbf{w}

**Conventional (Matched Filter) Beamformer**

The simplest beamformer sets weights equal to the steering vector of the desired direction:

.. math::

   \mathbf{w}_{\text{conv}}(\mathbf{u}_0) = \mathbf{s}(\mathbf{u}_0)

The gain in the target direction:

.. math::

   G_{\text{conv}}(\mathbf{u}_0) = |\mathbf{s}^H(\mathbf{u}_0) \mathbf{s}(\mathbf{u}_0)|^2 = N^2

The gain toward an off-target direction :math:`\mathbf{u}`:

.. math::

   G_{\text{conv}}(\mathbf{u}) = |\mathbf{s}^H(\mathbf{u}_0) \mathbf{s}(\mathbf{u})|^2

For an N-element ULA with half-wavelength spacing:

.. math::

   \mathbf{s}^H(\mathbf{u}_0) \mathbf{s}(\mathbf{u}) = \sum_{n=0}^{N-1} e^{j 2\pi d \sin(\Delta\theta) n / \lambda}

   = \frac{\sin(N \pi d \sin(\Delta\theta) / \lambda)}{\sin(\pi d \sin(\Delta\theta) / \lambda)}

where :math:`\Delta\theta = \theta - \theta_0` is the angular offset. This is a sinc-like function (the **array factor**).

**Beamwidth**

The **3-dB beamwidth** (half-power beam width) is the angular separation where the gain drops to half power (:math:`-3` dB). For a ULA with :math:`N` elements and half-wavelength spacing:

.. math::

   \Delta\theta_{3dB} \approx 1.2 \lambda / (N d) \text{ radians} = 69 \lambda / (N d) \text{ degrees}

This is the fundamental diffraction limit for antenna arrays.

**Sidelobe Level**

The conventional beamformer exhibits sidelobe artifacts—regions away from the main lobe where the gain is significantly above the background. The first sidelobe is approximately **-13 dB** relative to the main lobe.

---

**************************
MVDR Beamformer Theory
****************************

**Problem Formulation**

The MVDR (Minimum Variance Distortionless Response) beamformer solves the optimization problem:

.. math::

   \min_{\mathbf{w}} \mathbf{w}^H \mathbf{R} \mathbf{w}

   \text{subject to: } \mathbf{w}^H \mathbf{s}(\mathbf{u}_0) = 1

where :math:`\mathbf{u}_0` is the target direction.

**Interpretation:** Minimize the output power (variance) subject to unit gain in the target direction.

**Closed-Form Solution**

Using Lagrange multipliers, the optimal weight vector is:

.. math::

   \mathbf{w}_{\text{MVDR}} = \frac{\mathbf{R}^{-1} \mathbf{s}(\mathbf{u}_0)}{\mathbf{s}^H(\mathbf{u}_0) \mathbf{R}^{-1} \mathbf{s}(\mathbf{u}_0)}

**Output Power (Gain)**

The output power of the MVDR beamformer is:

.. math::

   P_{\text{MVDR}} = \mathbf{w}_{\text{MVDR}}^H \mathbf{R} \mathbf{w}_{\text{MVDR}} = \frac{1}{\mathbf{s}^H(\mathbf{u}_0) \mathbf{R}^{-1} \mathbf{s}(\mathbf{u}_0)}

**Equivalence to Wiener Filter**

The MVDR beamformer is equivalent to the Wiener filter for the specific case where:
- The noise is white (:math:`\mathbf{R}_\eta = \sigma_\eta^2 \mathbf{I}`)
- We desire unit gain in direction :math:`\mathbf{u}_0`

For this case:

.. math::

   \mathbf{w}_{\text{MVDR}} = \frac{\mathbf{R}^{-1} \mathbf{s}(\mathbf{u}_0)}{\mathbf{s}^H(\mathbf{u}_0) \mathbf{R}^{-1} \mathbf{s}(\mathbf{u}_0)} = \frac{(\mathbf{R}_s + \sigma_\eta^2 \mathbf{I})^{-1} \mathbf{s}(\mathbf{u}_0)}{\mathbf{s}^H(\mathbf{u}_0) (\mathbf{R}_s + \sigma_\eta^2 \mathbf{I})^{-1} \mathbf{s}(\mathbf{u}_0)}

**MVDR vs. Conventional**

**Gains:**
- MVDR adapts to the interference covariance and suppresses interference
- MVDR achieves better signal-to-interference-plus-noise ratio (SINR)
- MVDR creates deep nulls in interference directions

**Losses:**
- MVDR requires accurate covariance matrix estimation (requires many samples)
- MVDR gain in target direction is lower than conventional (typically 0 dB vs. :math:`10\log_{10}(N)` dB)
- MVDR is sensitive to mismatch between assumed and true signal direction

---

***********************
DOA Estimation Theory
***********************

**Problem Setup**

Given noisy observations :math:`\mathbf{r}(t_1), \ldots, \mathbf{r}(t_M)` from a mixture of :math:`K` plane waves, estimate the directions of arrival :math:`\mathbf{u}_1, \ldots, \mathbf{u}_K`.

**MUSIC Algorithm: Subspace Decomposition**

The MUSIC (MUltiple SIgnal Classification) algorithm exploits the orthogonality between signal subspace and noise subspace.

1. **Compute covariance:** :math:`\hat{\mathbf{R}} = \frac{1}{M} \sum_m \mathbf{r}(t_m) \mathbf{r}^H(t_m)`

2. **Eigendecomposition:** :math:`\hat{\mathbf{R}} = \sum_k \lambda_k \mathbf{v}_k \mathbf{v}_k^H` with :math:`\lambda_1 \geq \cdots \geq \lambda_N`

3. **Partition eigenvectors:**
   - Signal subspace: :math:`\mathbf{V}_s = [\mathbf{v}_1 \, | \, \cdots \, | \, \mathbf{v}_K]`
   - Noise subspace: :math:`\mathbf{V}_n = [\mathbf{v}_{K+1} \, | \, \cdots \, | \, \mathbf{v}_N]`

4. **MUSIC spectrum:**

.. math::

   P_{\text{MUSIC}}(\mathbf{u}) = \frac{1}{\mathbf{s}^H(\mathbf{u}) \mathbf{V}_n \mathbf{V}_n^H \mathbf{s}(\mathbf{u})}

5. **Find peaks:** The :math:`K` highest peaks of :math:`P_{\text{MUSIC}}(\mathbf{u})` correspond to the DOA estimates.

**Why MUSIC Works: Key Insight**

For true signal directions :math:`\mathbf{u}_k` (k=1,...,K), the steering vectors :math:`\mathbf{s}(\mathbf{u}_k)` lie in the signal subspace by definition:

.. math::

   \mathbf{R}_s = \sum_{k=1}^{K} P_k \mathbf{s}(\mathbf{u}_k) \mathbf{s}^H(\mathbf{u}_k)

Thus, :math:`\mathbf{s}(\mathbf{u}_k)` is orthogonal to the noise subspace:

.. math::

   \mathbf{V}_n^H \mathbf{s}(\mathbf{u}_k) = \mathbf{0}

At these directions, the MUSIC denominator is exactly zero, creating **peaks** (or **infinite gain**). At other directions, the steering vector has non-zero projection onto the noise subspace, so :math:`P_{\text{MUSIC}}(\mathbf{u})` is finite.

**Resolution Limit**

MUSIC can resolve two signals separated by:

.. math::

   \Delta\theta_{\text{min}} \approx \frac{\lambda}{2L}

where :math:`L` is the array **aperture size** (largest distance between elements). This is Rayleigh's criterion for angular resolution.

For a 3D array:

.. math::

   \Delta\theta_{\min} \approx \frac{\lambda}{2 \max(\text{array dimensions})}

**Cramer-Rao Lower Bound**

The CRB provides a lower bound on the variance of any unbiased DOA estimator. For a ULA with N elements in white noise:

.. math::

   \text{CRB}(\Delta\theta) = \frac{\lambda^2}{2\pi^2 (N^2-1) d^2 \sin^2(\theta) \cdot \text{SNR}}

Key insights:
- Variance decreases as :math:`1/\text{SNR}`
- Variance decreases as :math:`1/N^2` (quadratic with array size)
- Variance increases as :math:`1/d^2` (aperture size dominates resolution)
- Variance increases as :math:`1/\sin^2(\theta)` (harder to estimate angles near endfire)

---

**Spatial Aliasing and Sampling Theory**

**Nyquist Sampling Criterion for Arrays**

The array equivalent of the Nyquist sampling theorem states: to avoid **grating lobes**, the element spacing must satisfy:

.. math::

   d \leq \frac{\lambda}{2}

**Derivation:** For a ULA, the array factor at direction :math:`\theta` is:

.. math::

   \sum_{n=0}^{N-1} e^{j 2\pi d n \sin(\theta) / \lambda}

This is periodic in :math:`\sin(\theta)` with period :math:`\lambda / d`. If :math:`d > \lambda/2`, then the period is < 1, causing the main lobe to repeat within the visible range :math:`\sin(\theta) \in [-1, 1]`. These repeated lobes are **grating lobes**.

**Impact in 3D**

For 3D arrays, the Nyquist criterion applies to each pair of elements across each axis:

.. math::

   d_x, d_y, d_z \leq \frac{\lambda}{2}

A cubic array with :math:`d > \lambda/2` exhibits grating lobes in 3D space, complicating DOA estimation.

---

*****************************
Array Gain and Directivity
*****************************

**Directivity Definition**

The **directivity** of a beamformer is the ratio of power gain in the steered direction to the average power gain across all directions:

.. math::

   D = \frac{G(\mathbf{u}_0)}{(1/4\pi) \int_{\Omega} G(\mathbf{u}) d\Omega}

where the integral is over the entire 3D sphere of directions.

For a conventional beamformer with N isotropic elements:

.. math::

   D_{\text{conv}} = N

For MVDR, directivity is lower due to the unit-gain constraint (directivity is typically :math:`D_{\text{MVDR}} < D_{\text{conv}}`).

**Array Gain (Processing Gain)**

The **array gain** is the improvement in SNR achieved by beamforming:

.. math::

   G_{\text{array}} = \frac{\text{SNR}_{\text{out}}}{\text{SNR}_{\text{in}}} = \frac{P_{\text{signal}} / P_{\text{noise, out}}}{P_{\text{signal}} / P_{\text{noise, in}}}

For a conventional beamformer pointing at the signal direction:

.. math::

   G_{\text{array}} \approx N \quad \text{(assuming noise is white and signal is incoherent across elements)}

This is the **processing gain**: each element contributes independently, so array size provides :math:`N \times` power improvement (or :math:`10\log_{10}(N)` dB gain).

For MVDR in the presence of interference:

.. math::

   G_{\text{array}} = \frac{1}{\mathbf{s}^H(\mathbf{u}_0) \mathbf{R}^{-1}_{\eta + \text{int}} \mathbf{s}(\mathbf{u}_0)}

where :math:`\mathbf{R}_{\eta + \text{int}}` is the noise-plus-interference covariance. MVDR typically provides superior SNR improvement in high-interference scenarios.

---

**Beam Pattern and Sidelobe Control**

**Taper (Window) Functions**

To reduce sidelobe levels, weights can be modified using a **taper function**. Instead of:

.. math::

   \mathbf{w} = \mathbf{s}(\mathbf{u}_0)

we use:

.. math::

   \mathbf{w} = \text{diag}(\mathbf{t}) \mathbf{s}(\mathbf{u}_0)

where :math:`\mathbf{t}` is a taper vector with values :math:`t_n \in [0, 1]`.

**Common Tapers:**

1. **Uniform** (no taper): :math:`t_n = 1` → Sidelobe level = -13 dB
2. **Hanning:** :math:`t_n = 0.5 - 0.5\cos(2\pi n/(N-1))` → Sidelobe level = -43 dB
3. **Hamming:** :math:`t_n = 0.54 - 0.46\cos(2\pi n/(N-1))` → Sidelobe level = -53 dB
4. **Kaiser:** Tunable via shape parameter

**Trade-off:** Stronger tapering reduces sidelobe levels but increases main-lobe width (degrades resolution).

---

*****************************
Performance Limitations
*****************************

**Finite Sample Effects**

All DOA and beamforming algorithms require estimation of the covariance matrix from samples. With finite data, the estimated covariance :math:`\hat{\mathbf{R}}` differs from the true :math:`\mathbf{R}`:

.. math::

   \text{error} = \|\hat{\mathbf{R}} - \mathbf{R}\| \propto 1/\sqrt{M}

where M is the number of samples. Practical rule of thumb: :math:`M \geq 10N` samples required for reasonable estimation.

**Model Mismatch**

MVDR and MUSIC assume:
1. Plane waves (true in far field)
2. Known element positions exactly
3. Isotropic elements
4. White noise

Violations degrade performance:

- **Element position error (:math:`\Delta\mathbf{r}`):** Steering vector error :math:`\Delta\mathbf{s} \sim 2\pi \Delta\mathbf{r} / \lambda`. Error > :math:`\lambda/20` causes significant degradation.
- **Non-isotropic elements:** Element patterns vary with direction; algorithms assume unit magnitude response.
- **Colored noise:** MVDR can collapse if noise is correlated; requires more samples or diagonal loading.

**Mutual Coupling**

In compact arrays, electromagnetic coupling between elements distorts the received signal. This is not captured by the plane-wave/steering-vector model. Compensation requires:
- Calibration with known sources
- Electromagnetic simulation pre-computed
- Adaptive algorithms that learn coupling structure

---

**Summary of Fundamentals**

The mathematical foundation of 3D beamforming rests on:

1. **Plane wave propagation** — Far-field assumption, phase relationships
2. **Steering vectors** — Capture phase shifts, define array manifold
3. **Spatial covariance** — Encodes signal+noise+interference structure
4. **Subspace decomposition** — Enables signal vs. noise separation (MUSIC, ESPRIT)
5. **Optimization theory** — MVDR solves constrained minimization
6. **Sampling theory** — Nyquist criterion prevents aliasing
7. **Performance metrics** — Directivity, gain, resolution, CRB bounds

Understanding these fundamentals is essential for:
- Designing array geometries that meet performance targets
- Choosing algorithms appropriate for signal structure
- Recognizing failure modes (aliasing, mismatch, finite samples)
- Predicting and validating system performance

---

**References**

- Van Trees, H. L. (2002). *Optimum Array Processing: Part IV of Detection, Estimation, and Modulation Theory*. Wiley-Interscience.
- Kay, S. M. (1993). *Fundamentals of Statistical Signal Processing: Estimation Theory*. Prentice Hall.
- Stoica, P., & Moses, R. (2005). *Spectral Analysis of Signals*. Prentice Hall.
- Krim, H., & Viberg, M. (1996). "Two Decades of Array Signal Processing Research." *IEEE Signal Processing Magazine*, 13(4), 67-94.
- Orfanidis, S. J. (2010). *Electromagnetic Waves and Antennas*. [Online: ece.rutgers.edu/~orfanidi/ewa]
