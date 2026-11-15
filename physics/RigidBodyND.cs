// RigidBodyND.cs -- Rotational ("spin") dynamics of a rigid body in N dimensions.
//
// In N dimensions angular velocity and angular momentum are antisymmetric
// n x n matrices (bivectors). The body is described by its mass second-moment
// matrix  J = sum_i m_i * x_i * x_i^T  (body frame), and the inertia tensor is
// the linear map  Omega -> M = J*Omega + Omega*J.
// (In 3D: I = trace(J)*Id - J, i.e. J = 0.5*trace(I)*Id - I.)
//
// Equations of motion (Euler-Arnold on SO(n)), body frame:
//     dR/dt = R * Omega
//     dM/dt = M*Omega - Omega*M + T_body
using System;

public sealed class RigidBodyND
{
    public readonly int N;
    public double[,] R;   // orientation (body -> world), orthogonal
    public double[,] M;   // angular momentum in body frame, antisymmetric
    public double[,] J;   // mass second-moment matrix in body frame, SPD

    public RigidBodyND(double[,] J, double[,] M, double[,] R = null)
    {
        N = J.GetLength(0);
        this.J = (double[,])J.Clone();
        this.M = (double[,])M.Clone();
        this.R = R != null ? (double[,])R.Clone() : Identity(N);
    }

    /// <summary>Build a body from a classic 3x3 inertia tensor.</summary>
    public static double[,] JFromInertia3(double[,] I)
    {
        double h = 0.5 * (I[0, 0] + I[1, 1] + I[2, 2]);
        var J = new double[3, 3];
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                J[i, j] = (i == j ? h : 0.0) - I[i, j];
        return J;
    }

    /// <summary>Inertia tensor applied to a bivector: M = J*Omega + Omega*J.</summary>
    public static double[,] ApplyInertia(double[,] J, double[,] Omega)
    {
        int n = J.GetLength(0);
        var M = new double[n, n];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
            {
                double s = 0.0;
                for (int k = 0; k < n; k++)
                    s += J[i, k] * Omega[k, j] + Omega[i, k] * J[k, j];
                M[i, j] = s;
            }
        return M;
    }

    /// <summary>Angular velocity from angular momentum: solves J*Omega + Omega*J = M.</summary>
    public static double[,] AngularVelocity(double[,] J, double[,] M)
    {
        int n = J.GetLength(0), k = n * (n - 1) / 2;
        var A = new double[k, k];
        var b = new double[k, 1];

        int q = 0;
        for (int a = 0; a < n; a++)
            for (int c = a + 1; c < n; c++, q++)
            {
                var E = new double[n, n];
                E[a, c] = 1.0; E[c, a] = -1.0;
                var JE = ApplyInertia(J, E);
                int p = 0;
                for (int i = 0; i < n; i++)
                    for (int j = i + 1; j < n; j++, p++) A[p, q] = JE[i, j];
            }
        int r = 0;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++, r++) b[r, 0] = M[i, j];

        LinSolve(A, b);

        var W = new double[n, n];
        r = 0;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++, r++) { W[i, j] = b[r, 0]; W[j, i] = -b[r, 0]; }
        return W;
    }

    /// <summary>Spin rate (Euler's equations in N-D): dM/dt = M*Omega - Omega*M + Tb.</summary>
    /// <param name="Tb">Body-frame torque bivector, or null for torque-free.</param>
    public static double[,] SpinRate(double[,] J, double[,] M, double[,] Tb = null)
    {
        int n = J.GetLength(0);
        var W = AngularVelocity(J, M);
        var MW = Mul(M, W);
        var WM = Mul(W, M);
        var d = new double[n, n];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                d[i, j] = MW[i, j] - WM[i, j] + (Tb != null ? Tb[i, j] : 0.0);
        return d;
    }

    /// <summary>Rotational kinetic energy E = 1/4 * sum Omega_ij M_ij.</summary>
    public double KineticEnergy()
    {
        var W = AngularVelocity(J, M);
        double e = 0.0;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) e += W[i, j] * M[i, j];
        return 0.25 * e;
    }

    /// <summary>Body-frame angular velocity of this body.</summary>
    public double[,] BodyAngularVelocity() => AngularVelocity(J, M);

    /// <summary>World-frame angular velocity: R * Omega * R^T.</summary>
    public double[,] WorldAngularVelocity() => Mul(Mul(R, AngularVelocity(J, M)), Transpose(R));

    /// <summary>
    /// Advance the spin by dt: RK4 on M, Cayley transform on R (stays exactly orthogonal).
    /// </summary>
    public void Step(double dt, double[,] Tb = null)
    {
        int n = N;
        var M0 = (double[,])M.Clone();
        var k1 = SpinRate(J, M0, Tb);
        var k2 = SpinRate(J, AddScaled(M0, k1, 0.5 * dt), Tb);
        var k3 = SpinRate(J, AddScaled(M0, k2, 0.5 * dt), Tb);
        var k4 = SpinRate(J, AddScaled(M0, k3, dt), Tb);
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                M[i, j] = M0[i, j] + dt / 6.0 * (k1[i, j] + 2 * k2[i, j] + 2 * k3[i, j] + k4[i, j]);

        var Mmid = new double[n, n];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) Mmid[i, j] = 0.5 * (M0[i, j] + M[i, j]);
        var W = AngularVelocity(J, Mmid);

        var A = new double[n, n];
        var Q = new double[n, n];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
            {
                double h = 0.5 * dt * W[i, j], d = i == j ? 1.0 : 0.0;
                A[i, j] = d - h;
                Q[i, j] = d + h;
            }
        LinSolve(A, Q);           // Q = (Id - h W)^-1 (Id + h W)
        R = Mul(R, Q);
    }

    // ---------------- helpers ----------------
    static double[,] Identity(int n)
    {
        var I = new double[n, n];
        for (int i = 0; i < n; i++) I[i, i] = 1.0;
        return I;
    }

    static double[,] Mul(double[,] A, double[,] B)
    {
        int n = A.GetLength(0), m = B.GetLength(1), kk = A.GetLength(1);
        var C = new double[n, m];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++)
            {
                double s = 0.0;
                for (int k = 0; k < kk; k++) s += A[i, k] * B[k, j];
                C[i, j] = s;
            }
        return C;
    }

    static double[,] Transpose(double[,] A)
    {
        int n = A.GetLength(0), m = A.GetLength(1);
        var T = new double[m, n];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++) T[j, i] = A[i, j];
        return T;
    }

    static double[,] AddScaled(double[,] A, double[,] B, double s)
    {
        int n = A.GetLength(0), m = A.GetLength(1);
        var C = new double[n, m];
        for (int i = 0; i < n; i++)
            for (int j = 0; j < m; j++) C[i, j] = A[i, j] + s * B[i, j];
        return C;
    }

    /// <summary>Solves A X = B in place (B becomes X). A is destroyed.</summary>
    static void LinSolve(double[,] A, double[,] B)
    {
        int k = A.GetLength(0), nr = B.GetLength(1);
        for (int c = 0; c < k; c++)
        {
            int p = c;
            for (int r = c + 1; r < k; r++)
                if (Math.Abs(A[r, c]) > Math.Abs(A[p, c])) p = r;
            if (Math.Abs(A[p, c]) < 1e-300)
                throw new InvalidOperationException("Singular inertia / matrix.");
            if (p != c)
            {
                for (int j = 0; j < k; j++) (A[c, j], A[p, j]) = (A[p, j], A[c, j]);
                for (int j = 0; j < nr; j++) (B[c, j], B[p, j]) = (B[p, j], B[c, j]);
            }
            for (int r = c + 1; r < k; r++)
            {
                double f = A[r, c] / A[c, c];
                if (f == 0.0) continue;
                for (int j = c; j < k; j++) A[r, j] -= f * A[c, j];
                for (int j = 0; j < nr; j++) B[r, j] -= f * B[c, j];
            }
        }
        for (int c = k - 1; c >= 0; c--)
            for (int j = 0; j < nr; j++)
            {
                double s = B[c, j];
                for (int m = c + 1; m < k; m++) s -= A[c, m] * B[m, j];
                B[c, j] = s / A[c, c];
            }
    }
}
