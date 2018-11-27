#
# tlibs2 julia module
# @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
# @date 2017 -- 2018
# @license GPLv3, see 'LICENSE' file
# @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
#

__precompile__()
module tl2


t_real = Float64
g_enable_debug = 1


#
# initialises tlibs2
#
function __init__()
	ccall((:load_tl2, :tl2jl), Void, (Cint,), g_enable_debug)
end



#
# loads instrument data files
#
function loadinstr(strFile::String) :: Array{Any, 1}
	scandata = ccall((:load_instr, :tl2jl), Array{Any, 1}, (Cstring,), strFile)
	if length(scandata) == 0 return scandata end

	thedict = Dict{String,String}()
	for (key, val) in zip(scandata[3], scandata[4])
		thedict[key] = val
	end

	# header names, data matrix, property dict
	return [ scandata[1], scandata[2], thedict ]
end



#
# function fitting
#
function fit(fkt, x, y, yerr; fixed = [], values = Dict(), errors = Dict())
	# find number of function arguments
	meth = methods(fkt).ms[1]
	num_args = meth.sig.parameters.length - 1
	num_free_params = num_args - 1


	# get function argument names
	strMeth = repr(meth)
	strArgs = strMeth[searchindex(strMeth, "(")+1 : searchindex(strMeth, ")")-1]
	arrArgs = map(strip, split(strArgs, ","))
	arrArgs = map(String, arrArgs)
	arrParams = arrArgs[2 : length(arrArgs)]	# only free params


	# build values/errors array
	iParam = 0
	arrValues = Array{t_real, 1}(num_free_params)
	arrErrors = Array{t_real, 1}(num_free_params)

	for strArg in arrArgs
		# skip "x" parameter
		if(iParam == 0)
			iParam += 1
			continue
		end

		valArg = get(values, strArg, 0.)
		errArg = get(errors, strArg, valArg*0.1)
		
		arrValues[iParam] = valArg
		arrErrors[iParam] = errArg
		iParam += 1
	end

	# map to a C function pointer with "num_args" arguments
	cfkt = cfunction(fkt, t_real, NTuple{num_args, t_real})

	# call C function pointer
	bOk = ccall((:fit, :tl2jl),
		# return type
		Cint,

		# arg types
		(Ptr{Void}, Csize_t,
		Ptr{t_real}, Ptr{t_real}, Ptr{t_real}, Csize_t,
		Array{String, 1}, Array{String, 1},
		Ptr{t_real}, Ptr{t_real}),

		# args
		cfkt, num_free_params, x, y, yerr, length(x), arrArgs, fixed, arrValues, arrErrors)



	# build map of values & errors
	dictRet = Dict()

	for (strParam, valArg, valErr) in zip(arrParams, arrValues, arrErrors)
		dictRet[strParam] = valArg
		dictRet[strParam * "_err"] = valErr
	end

	dictRet["<valid>"] = bOk
	dictRet["<func>"] = fkt
	dictRet["<args>"] = arrParams
	dictRet["<args_val>"] = arrValues
	dictRet["<args_err>"] = arrErrors
	return dictRet
end



#
# function minimisation
#
function minimise(fkt; fixed = [], values = Dict(), errors = Dict())
	# find number of function arguments
	meth = methods(fkt).ms[1]
	num_args = meth.sig.parameters.length - 1


	# get function argument names
	strMeth = repr(meth)
	strArgs = strMeth[searchindex(strMeth, "(")+1 : searchindex(strMeth, ")")-1]
	arrArgs = map(strip, split(strArgs, ","))
	arrArgs = map(String, arrArgs)
	arrParams = arrArgs[1 : length(arrArgs)]


	# build values/errors array
	arrValues = Array{t_real, 1}(num_args)
	arrErrors = Array{t_real, 1}(num_args)

	iParam = 1
	for strArg in arrArgs
		valArg = get(values, strArg, 0.)
		errArg = get(errors, strArg, valArg*0.1)
		
		arrValues[iParam] = valArg
		arrErrors[iParam] = errArg
		iParam += 1
	end

	# map to a C function pointer with "num_args" arguments
	cfkt = cfunction(fkt, t_real, NTuple{num_args, t_real})


	# call C function pointer
	bOk = ccall((:minimise, :tl2jl),
		# return type
		Cint,

		# arg types
		(Ptr{Void}, Csize_t,
		Array{String, 1}, Array{String, 1},
		Ptr{t_real}, Ptr{t_real}),

		# args
		cfkt, num_args, arrArgs, fixed, arrValues, arrErrors)



	# build map of values & errors
	dictRet = Dict()

	for (strParam, valArg, valErr) in zip(arrParams, arrValues, arrErrors)
		dictRet[strParam] = valArg
		dictRet[strParam * "_err"] = valErr
	end

	dictRet["<valid>"] = bOk
	dictRet["<func>"] = fkt
	dictRet["<args>"] = arrParams
	dictRet["<args_val>"] = arrValues
	dictRet["<args_err>"] = arrErrors
	return dictRet
end



#
# evaluate the fitted function using the fitted parameters
#
function eval_func(fitresult, xmin, xmax, count=128)
	fkt = fitresult["<func>"]

	xs = linspace(xmin, xmax, count)
	ys = fkt(xs, fitresult["<args_val>"]...)

	return [ Array(xs), Array(ys) ]
end



#
# Gauss model
#
function gauss_model_amp(x, x0, sigma, amp, offs)
	return amp * exp.(-0.5 * ((x-x0)/sigma).*((x-x0)/sigma)) + offs
end


#
# Lorentz model
#
function lorentz_model_amp(x, x0, hwhm, amp, offs)
	return amp*hwhm*hwhm / ((x-x0)*(x-x0) + hwhm*hwhm) + offs
end



#
# Poisson error
#
function poisson_err(x) :: t_real
	if(x == 0)
		return 1
	end

	return sqrt(x)
end


#
# Conversions
#
SIGMA2HWHM = sqrt(2.*log(2.))
SIGMA2FWHM = 2.*SIGMA2HWHM
HWHM2SIGMA = 1./SIGMA2HWHM
FWHM2SIGMA = 1./SIGMA2FWHM


end		# module tl2
