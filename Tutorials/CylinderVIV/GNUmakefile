# Makefile for Cylinder VIV tutorial

# AMReX
DIM = 3
COMP = gnu
USE_MPI = FALSE
USE_OMP = FALSE
USE_CUDA = FALSE

# IAMR specific
USE_PARTICLES = TRUE
# USE_EB = TRUE  # 注释掉或删除

# Include the main Makefile
include ../../Exec/Make.IAMR

# Additional flags for VIV simulation
CXXFLAGS += -DAMREX_PARTICLES
CXXFLAGS += -DUSE_CYLINDER_VIV
# 不要加-DAMREX_USE_EB 