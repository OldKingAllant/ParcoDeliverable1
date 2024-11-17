import matplotlib as mp
import matplotlib.pyplot as plt
import math
import sys

def parse_threads(input_file, n_rep):
	threads = []
	while n_rep > 0:
		line = input_file.readline().rstrip()
		sep = line.split(' ')
		n_threads = int(sep[0])
		took = float(sep[1])
		threads.append({'threads': n_threads, 'time': took})
		n_rep -= 1
	return threads

def parse_section(input_file, n_rep):
	curr_n = input_file.readline().rstrip()

	if curr_n == '':
		return False

	curr_n = int(curr_n)
	sym_check = float(input_file.readline().rstrip())
	imp_check = float(input_file.readline().rstrip())
	omp_check = parse_threads(input_file, n_rep)
	transpose = float(input_file.readline().rstrip())
	imp_transpose = float(input_file.readline().rstrip())
	omp_transpose = parse_threads(input_file, n_rep)
	obv_transpose = float(input_file.readline().rstrip())
	obv_omp_transpose = parse_threads(input_file, n_rep)
	final_omp_transpose = parse_threads(input_file, n_rep)

	section = {'n': curr_n, 'sym': sym_check, 'imp_sym': imp_check}

	section['omp_sym'] = omp_check
	section['transpose'] = transpose
	section['imp_transpose'] = imp_transpose
	section['omp_transpose'] = omp_transpose
	section['obv_transpose'] = obv_transpose
	section['obv_omp_transpose'] = obv_omp_transpose
	section['final_omp_transpose'] = final_omp_transpose

	return section

def parse_file(input_file):
	max_n = int(input_file.readline().rstrip())
	max_threads = int(input_file.readline().rstrip())
	data = { 'max_n': max_n, 'threads': max_threads }

	num_rep_per_n = int(math.log2(max_threads))
	
	benchmarks = []

	while True:
		new_data = parse_section(input_file, num_rep_per_n)
		if new_data == False:
			break
		benchmarks.append(new_data)

	data['benchmarks'] = benchmarks

	return data

#Confront baseline symmetry check
#with other implementations 
#depending on N
def output_symmetry(data, n_threads):
	benchmarks = data['benchmarks']

	collect_n = [benchmarks[i]['n'] for i in range(len(benchmarks))]
	collect_sym_check = [benchmarks[i]['sym'] for i in range(len(benchmarks))]
	collect_imp_sym = [benchmarks[i]['imp_sym'] for i in range(len(benchmarks))]
	collect_omp_sym = [list(filter(lambda entry: (entry['threads'] == n_threads), benchmarks[i]['omp_sym']))[0]['time'] for i in range(len(benchmarks))]

	#y axis: time
	#x axis: N
	plt.title('Symmetry check')
	plt.ylabel('Time (ms)')
	plt.xlabel('Matrix size (sqrt(N) elements)')
	line_sym_check = plt.plot(collect_n, collect_sym_check, '^-b', label='Base sym')
	line_imp_check = plt.plot(collect_n, collect_imp_sym, 's-r', label='Imp sym')
	line_omp_check = plt.plot(collect_n, collect_omp_sym, 'D-g', label=f'OMP sym {n_threads} threads')
	plt.legend()
	plt.savefig('sym.png')

def output_transpose(data, n_threads):
	benchmarks = data['benchmarks']

	collect_n = [benchmarks[i]['n'] for i in range(len(benchmarks))]
	collect_transpose = [benchmarks[i]['transpose'] for i in range(len(benchmarks))]
	collect_imp_transpose = [benchmarks[i]['imp_transpose'] for i in range(len(benchmarks))]
	collect_obv_transpose = [benchmarks[i]['obv_transpose'] for i in range(len(benchmarks))]

	collect_omp_transpose = [list(filter(lambda entry: (entry['threads'] == n_threads), benchmarks[i]['omp_transpose']))[0]['time'] for i in range(len(benchmarks))]
	collect_obv_omp_transpose = [list(filter(lambda entry: (entry['threads'] == n_threads), benchmarks[i]['obv_omp_transpose']))[0]['time'] for i in range(len(benchmarks))]
	collect_final_transpose = [list(filter(lambda entry: (entry['threads'] == n_threads), benchmarks[i]['final_omp_transpose']))[0]['time'] for i in range(len(benchmarks))]

	plt.clf()
	plt.title('Transpose')
	plt.ylabel('Time (ms)')
	plt.xlabel('Matrix size (sqrt(N) elements)')

	line_transose = plt.plot(collect_n, collect_transpose, '^-b', label='Base transpose')
	line_imp_tran = plt.plot(collect_n, collect_imp_transpose, 's-r', label='Imp transpose')
	line_obv_tran = plt.plot(collect_n, collect_obv_transpose, 'D-g', label='Oblivious transpose')
	line_omp_tran = plt.plot(collect_n, collect_omp_transpose, 'o-y', label='OMP transpose')
	line_omp_obv = plt.plot(collect_n, collect_obv_omp_transpose, '*-k', label='OMP Oblivious')

	plt.legend()
	plt.savefig('trans.png')
	return

def output_compare_symmetry(data):
	benchmarks = data['benchmarks']

	collect_n = [benchmarks[i]['n'] for i in range(len(benchmarks))]

	max_threads = data['threads']
	n_lines = int(math.log2(max_threads))

	plt.clf()
	plt.title('Symmetry with different n_threads')
	plt.ylabel('Time (ms)')
	plt.xlabel('Matrix size (sqrt(N) elements)')

	for thread_id in range(n_lines):
		collect_sym_per_thread = [benchmarks[i]['omp_sym'][thread_id]['time'] for i in range(len(benchmarks))]
		plt.plot(collect_n, collect_sym_per_thread, label=f'{2**(thread_id+1)} threads')

	plt.legend()
	plt.savefig('compare_symm.png')
	return

def output_compare_transpose(data, name, alg_id):
	benchmarks = data['benchmarks']

	collect_n = [benchmarks[i]['n'] for i in range(len(benchmarks))]

	max_threads = data['threads']
	n_lines = int(math.log2(max_threads))

	plt.clf()
	plt.title(f'{name} with different n_threads')
	plt.ylabel('Time (ms)')
	plt.xlabel('Matrix size (sqrt(N) elements)')

	for thread_id in range(n_lines):
		collect_trans_per_thread = [benchmarks[i][alg_id][thread_id]['time'] for i in range(len(benchmarks))]
		plt.plot(collect_n, collect_trans_per_thread, label=f'{2**(thread_id+1)} threads')

	plt.legend()
	plt.savefig(f'{alg_id}.png')
	return

def generate():
	if len(sys.argv) < 3:
		return
	print(f'Using input file {sys.argv[1]}')
	with open(sys.argv[1], 'r') as input_file:
		data = parse_file(input_file)
		print(data)
		n_threads = int(sys.argv[2])
		output_symmetry(data, n_threads)
		output_transpose(data, n_threads)
		output_compare_symmetry(data)
		output_compare_transpose(data, 'OMP transpose', 'omp_transpose')
		output_compare_transpose(data, 'Oblivious OMP transpose', 'obv_omp_transpose')
		output_compare_transpose(data, 'Final transpose', 'final_omp_transpose')
	return

if __name__ == '__main__':
	generate()