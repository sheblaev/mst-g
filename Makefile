# defines
CC=g++
MPICC=mpicc
CXX=g++
MPICXX=mpicxx
CFLAGS= -O3 -Wall -std=gnu99 -openmp 
CXXFLAGS= -O3 -Wall
LDFLAGS= -O3 -lrt 


TARGET = mst_reference gen_valid_info validation gen_RMAT gen_SSCA2 mst_mpi mst 

all: $(TARGET)

# your own implementation, executable must called mst
mst: main.o mst.o graph_tools.o
	$(CXX) $^ -o $@ $(LDFLAGS)

mst_mpi: main_mpi.mpi.o graph_tools.mpi.o gen_RMAT_mpi.mpi.o mst.mpi.o gen_SSCA2_mpi.mpi.o 
	$(MPICXX) $^ -o $@ $(LDFLAGS)


# reference implementation	
mst_reference: main.o mst_reference.o graph_tools.o
	$(CXX) $^ -o $@ $(LDFLAGS)

gen_RMAT: gen_RMAT.o
	$(MPICXX) $^ -o $@ $(LDFLAGS)

gen_SSCA2: gen_SSCA2.o graph_tools.o
	$(MPICXX) $^ -o $@ $(LDFLAGS)

validation: validation.o graph_tools.o
	$(CXX) $^ -o $@ $(LDFLAGS)

gen_valid_info: graph_tools.o mst_reference.o gen_valid_info.o
	$(CXX) $^ -o $@ $(LDFLAGS)

%.mpi.o: %.cpp
	$(MPICXX) -DUSE_MPI $(CXXFLAGS) -o $@ -c $<

%.mpi.o: %.c
	$(MPICC) -DUSE_MPI $(CFLAGS) -o $@ -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o $(TARGET)

