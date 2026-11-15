/*
 * rigidbody_nd.c -- Rotational ("spin") dynamics of a rigid body in N dimensions.
 *
 * In N dimensions angular velocity and angular momentum are not vectors but
 * antisymmetric n x n matrices (bivectors). The body is described by its
 * mass second-moment matrix
 *
 *      J = sum_i  m_i * x_i * x_i^T        (body frame, symmetric, n x n)
 *
 * and the inertia tensor is the linear map  Omega -> M = J*Omega + Omega*J.
 * (In 3D this is exactly the usual I*omega, with I = trace(J)*Id - J,
 *  i.e. J = 0.5*trace(I)*Id - I.)
 *
 * Equations of motion (Euler-Arnold equations on SO(n)), body frame:
 *      dR/dt = R * Omega
 *      dM/dt = M*Omega - Omega*M + T_body
 *
 * All matrices are row-major double arrays of size n*n.
 * Compile:  gcc -O2 -std=c99 rigidbody_nd.c -lm
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    int     n;   /* number of spatial dimensions                         */
    double *R;   /* n*n orientation (body -> world), orthogonal           */
    double *M;   /* n*n angular momentum in body frame, antisymmetric     */
    double *J;   /* n*n mass second-moment matrix in body frame, SPD      */
} RigidBodyND;

/* C = A * B  (n x n). C must not alias A or B. */
static void mat_mul(int n, const double *A, const double *B, double *C)
{
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            double s = 0.0;
            for (int k = 0; k < n; ++k) s += A[i*n + k] * B[k*n + j];
            C[i*n + j] = s;
        }
}

/* Solve A X = B in place (A is k x k, B is k x nrhs). Partial pivoting.
 * On return B holds X. Returns 0 on success, -1 if A is singular. */
static int lin_solve(int k, double *A, double *B, int nrhs)
{
    for (int c = 0; c < k; ++c) {
        int p = c;
        for (int r = c + 1; r < k; ++r)
            if (fabs(A[r*k + c]) > fabs(A[p*k + c])) p = r;
        if (fabs(A[p*k + c]) < 1e-300) return -1;
        if (p != c) {
            for (int j = 0; j < k; ++j)    { double t = A[c*k+j]; A[c*k+j] = A[p*k+j]; A[p*k+j] = t; }
            for (int j = 0; j < nrhs; ++j) { double t = B[c*nrhs+j]; B[c*nrhs+j] = B[p*nrhs+j]; B[p*nrhs+j] = t; }
        }
        for (int r = c + 1; r < k; ++r) {
            double f = A[r*k + c] / A[c*k + c];
            if (f == 0.0) continue;
            for (int j = c; j < k; ++j)    A[r*k + j]    -= f * A[c*k + j];
            for (int j = 0; j < nrhs; ++j) B[r*nrhs + j] -= f * B[c*nrhs + j];
        }
    }
    for (int c = k - 1; c >= 0; --c)
        for (int j = 0; j < nrhs; ++j) {
            double s = B[c*nrhs + j];
            for (int m = c + 1; m < k; ++m) s -= A[c*k + m] * B[m*nrhs + j];
            B[c*nrhs + j] = s / A[c*k + c];
        }
    return 0;
}

/* 3D convenience: convert a classic 3x3 inertia tensor I to J. */
void rb_J_from_inertia3(const double I[9], double J[9])
{
    double h = 0.5 * (I[0] + I[4] + I[8]);
    for (int i = 0; i < 9; ++i) J[i] = -I[i];
    J[0] += h; J[4] += h; J[8] += h;
}

/* Inertia tensor applied to a bivector: M = J*Omega + Omega*J. */
void rb_apply_inertia(int n, const double *J, const double *Omega, double *M)
{
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            double s = 0.0;
            for (int k = 0; k < n; ++k)
                s += J[i*n + k] * Omega[k*n + j] + Omega[i*n + k] * J[k*n + j];
            M[i*n + j] = s;
        }
}

/* Angular velocity from angular momentum: solve J*Omega + Omega*J = M
 * for antisymmetric Omega (inverse of the inertia tensor).
 * Returns 0 on success, -1 on failure (singular J or out of memory). */
int rb_angular_velocity(int n, const double *J, const double *M, double *Omega)
{
    int k = n * (n - 1) / 2;          /* independent rotation planes */
    double *A  = (double *)malloc(sizeof(double) * (size_t)(k * k + k + 2 * n * n));
    if (!A) return -1;
    double *b  = A + k * k;
    double *E  = b + k;
    double *JE = E + n * n;

    /* Build the k x k matrix of the inertia operator column by column,
       using the basis bivectors E_ab (E[a][b] = 1, E[b][a] = -1). */
    int q = 0;
    for (int a = 0; a < n; ++a)
        for (int c = a + 1; c < n; ++c, ++q) {
            memset(E, 0, sizeof(double) * (size_t)(n * n));
            E[a*n + c] = 1.0; E[c*n + a] = -1.0;
            rb_apply_inertia(n, J, E, JE);
            int p = 0;
            for (int i = 0; i < n; ++i)
                for (int j = i + 1; j < n; ++j, ++p) A[p*k + q] = JE[i*n + j];
        }
    int p = 0;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j, ++p) b[p] = M[i*n + j];

    int ok = lin_solve(k, A, b, 1);
    if (ok == 0) {
        memset(Omega, 0, sizeof(double) * (size_t)(n * n));
        p = 0;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j, ++p) {
                Omega[i*n + j] =  b[p];
                Omega[j*n + i] = -b[p];
            }
    }
    free(A);
    return ok;
}

/* Spin rate (Euler's equations in N-D): dM/dt = M*Omega - Omega*M + Tb.
 * Tb is the torque bivector in the body frame (may be NULL for torque-free).
 * Optionally returns Omega (pass NULL if not needed). */
int rb_spin_rate(int n, const double *J, const double *M, const double *Tb,
                 double *dMdt, double *OmegaOut)
{
    double *W = (double *)malloc(sizeof(double) * (size_t)(3 * n * n));
    if (!W) return -1;
    double *MW = W + n * n, *WM = MW + n * n;
    if (rb_angular_velocity(n, J, M, W) != 0) { free(W); return -1; }
    mat_mul(n, M, W, MW);
    mat_mul(n, W, M, WM);
    for (int i = 0; i < n * n; ++i)
        dMdt[i] = MW[i] - WM[i] + (Tb ? Tb[i] : 0.0);
    if (OmegaOut) memcpy(OmegaOut, W, sizeof(double) * (size_t)(n * n));
    free(W);
    return 0;
}

/* Rotational kinetic energy  E = 1/4 * sum_ij Omega_ij * M_ij
 * (equals 1/2 * omega . (I omega) in 3D). */
double rb_kinetic_energy(int n, const double *J, const double *M)
{
    double *W = (double *)malloc(sizeof(double) * (size_t)(n * n));
    if (!W || rb_angular_velocity(n, J, M, W) != 0) { free(W); return NAN; }
    double e = 0.0;
    for (int i = 0; i < n * n; ++i) e += W[i] * M[i];
    free(W);
    return 0.25 * e;
}

/* Advance the body's spin by dt.
 *   - M is integrated with classic RK4.
 *   - R is updated with the Cayley transform using the midpoint Omega,
 *     so R stays exactly orthogonal (no drift / re-normalisation needed).
 * Tb: body-frame torque bivector, or NULL. Returns 0 on success. */
int rb_step(RigidBodyND *b, const double *Tb, double dt)
{
    int n = b->n, nn = n * n;
    double *buf = (double *)malloc(sizeof(double) * (size_t)(9 * nn));
    if (!buf) return -1;
    double *k1 = buf, *k2 = k1 + nn, *k3 = k2 + nn, *k4 = k3 + nn;
    double *tmp = k4 + nn, *Mold = tmp + nn, *W = Mold + nn;
    double *A = W + nn, *Q = A + nn;
    int err = 0;

    memcpy(Mold, b->M, sizeof(double) * (size_t)nn);

    /* --- RK4 on angular momentum --- */
    err |= rb_spin_rate(n, b->J, Mold, Tb, k1, NULL);
    for (int i = 0; i < nn; ++i) tmp[i] = Mold[i] + 0.5 * dt * k1[i];
    err |= rb_spin_rate(n, b->J, tmp, Tb, k2, NULL);
    for (int i = 0; i < nn; ++i) tmp[i] = Mold[i] + 0.5 * dt * k2[i];
    err |= rb_spin_rate(n, b->J, tmp, Tb, k3, NULL);
    for (int i = 0; i < nn; ++i) tmp[i] = Mold[i] + dt * k3[i];
    err |= rb_spin_rate(n, b->J, tmp, Tb, k4, NULL);
    if (err) { free(buf); return -1; }
    for (int i = 0; i < nn; ++i)
        b->M[i] = Mold[i] + dt / 6.0 * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);

    /* --- Orientation: R <- R * cay(Omega_mid * dt) --- */
    for (int i = 0; i < nn; ++i) tmp[i] = 0.5 * (Mold[i] + b->M[i]);
    if (rb_angular_velocity(n, b->J, tmp, W) != 0) { free(buf); return -1; }
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            double h = 0.5 * dt * W[i*n + j], d = (i == j) ? 1.0 : 0.0;
            A[i*n + j] = d - h;   /* Id - h*Omega */
            Q[i*n + j] = d + h;   /* Id + h*Omega */
        }
    if (lin_solve(n, A, Q, n) != 0) { free(buf); return -1; }  /* Q = A^-1 (Id + h*Omega) */
    mat_mul(n, b->R, Q, tmp);
    memcpy(b->R, tmp, sizeof(double) * (size_t)nn);

    free(buf);
    return 0;
}

/* World-frame angular velocity: Omega_world = R * Omega_body * R^T. */
int rb_world_angular_velocity(const RigidBodyND *b, double *OmegaWorld)
{
    int n = b->n, nn = n * n;
    double *W = (double *)malloc(sizeof(double) * (size_t)(2 * nn));
    if (!W) return -1;
    double *T = W + nn;
    if (rb_angular_velocity(n, b->J, b->M, W) != 0) { free(W); return -1; }
    mat_mul(n, b->R, W, T);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            double s = 0.0;
            for (int k = 0; k < n; ++k) s += T[i*n + k] * b->R[j*n + k];
            OmegaWorld[i*n + j] = s;
        }
    free(W);
    return 0;
}

#ifdef RIGIDBODY_ND_DEMO
#include <stdio.h>
int main(void)
{
    /* 4D box-like body spinning in two planes, torque-free. */
    enum { N = 4 };
    double R[N*N] = {0}, M[N*N] = {0}, J[N*N] = {0};
    double d[N] = {4.0, 3.0, 2.0, 1.0};
    for (int i = 0; i < N; ++i) { R[i*N+i] = 1.0; J[i*N+i] = d[i]; }
    M[0*N+1] = 2.0;  M[1*N+0] = -2.0;   /* spin in x-y plane */
    M[2*N+3] = 1.0;  M[3*N+2] = -1.0;   /* spin in z-w plane */
    M[0*N+2] = 0.1;  M[2*N+0] = -0.1;   /* small wobble      */
    RigidBodyND body = { N, R, M, J };

    printf("E0 = %.12f\n", rb_kinetic_energy(N, J, M));
    for (int s = 0; s < 1000; ++s) rb_step(&body, NULL, 0.01);
    printf("E  = %.12f\n", rb_kinetic_energy(N, J, M));
    return 0;
}
#endif
