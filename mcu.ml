(** mcu.ml -- Magnetic Card Utility. *)


(** Integer power operation.

	@param x mantissa.
	@param y exponent.
	@returns x power y.
*)
let rec pow x y =
	match y with
	| 0 -> 1
	| _ -> x * (pow x (pred y))
	;;


(* Constants. *)
let iata_char_length = 79;;
let iata_bits = 7;;
let iata_vbits = pred iata_bits;;
let iata_bit_length = iata_char_length * iata_bits;;
let iata_start_sentinel = "1010001";;
let iata_end_sentinel = "1111100";;
let iata_charset = 
	[|' '; '!'; '"'; '#'; '$'; '%'; '&'; '\'';
	  '('; ')'; '*'; '+'; ','; '-'; '.'; '/';
	  '0'; '1'; '2'; '3'; '4'; '5'; '6'; '7';
	  '8'; '9'; ':'; ';'; '<'; '='; '>'; '?';
	  '@'; 'A'; 'B'; 'C'; 'D'; 'E'; 'F'; 'G';
	  'H'; 'I'; 'J'; 'K'; 'L'; 'M'; 'N'; 'O';
	  'P'; 'Q'; 'R'; 'S'; 'T'; 'U'; 'V'; 'W';
	  'X'; 'Y'; 'Z'; '['; '\\'; ']'; '^'; '_'|];;
let iata_charset_begin = 32;;
let iata_b2c = Hashtbl.create (pow 2 iata_vbits);;
let iata_c2b = Hashtbl.create (pow 2 iata_vbits);;
let iata_p = Hashtbl.create (pow 2 iata_vbits);;

let aba_char_length = 40;;
let aba_bits = 5;;
let aba_vbits = pred aba_bits;;
let aba_bit_length = aba_char_length * aba_bits;;
let aba_start_sentinel = "11010";;
let aba_end_sentinel = "11111";;
let aba_charset =
	[|'0'; '1'; '2'; '3'; '4'; '5'; '6'; '7';
	  '8'; '9'; ':'; ';'; '<'; '='; '>'; '?'|];;
let aba_charset_begin = 48;;
let aba_b2c = Hashtbl.create (pow 2 aba_vbits);;
let aba_c2b = Hashtbl.create (pow 2 aba_vbits);;
let aba_p = Hashtbl.create (pow 2 aba_vbits);;

let thrift_char_length = 107;;
let thrift_bits = 5;;
let thrift_vbits = pred thrift_bits;;
let thrift_bit_length = thrift_char_length * thrift_bits;;
let thrift_start_sentinel = "11010";;
let thrift_end_sentinel = "11111";;
let thrift_charset =
	[|'0'; '1'; '2'; '3'; '4'; '5'; '6'; '7';
	  '8'; '9'; ':'; ';'; '<'; '='; '>'; '?'|];;
let thrift_charset_begin = 48;;
let thrift_b2c = Hashtbl.create (pow 2 thrift_vbits);;
let thrift_c2b = Hashtbl.create (pow 2 thrift_vbits);;
let thrift_p = Hashtbl.create (pow 2 thrift_vbits);;





(** Find sentinel.

	@param bitstring string with a bit stream.
	@param sentinel sentinel which marks position of data.
	@param start position from which to begin the search.
	@param bitstep how many bits to skip in each searching step.
	@raise Not_found if no sentinel found.
	@return position of the sentinel in the bit stream.
*)
let find_sentinel bitstring sentinel start bitstep =
	let sentinel_length = String.length sentinel in
	let searching_mark = (String.length bitstring) - sentinel_length in
	let rec searching_loop i =
		if i >= searching_mark then
			raise Not_found
		else
			let s = String.sub bitstring i sentinel_length in
			if s = sentinel then
				i
			else
				searching_loop (i + bitstep)
	in
	searching_loop start
	;;

(** Calculate (odd) parity of the value represented by a string.

	@param s bit string to calculate the parity of.
	@return parity.
*)
let calculate_parity s =
	let length = String.length s in
	let rec count_set_bits i value =
		if i == length then
			value
		else
			let next_value =
				if (String.get s i) == '1' then
					succ value
				else
					value
			in
			count_set_bits (succ i) next_value
	in
	let set_bits = count_set_bits 0 0 in
	let odd_set_bits = set_bits mod 2 in
	let parity =
		match odd_set_bits with
		| 0 -> '1'
		| 1 -> '0'
		| _ -> raise (Failure "character is neither 0 nor 1")
	in
	parity
	;;


(** Check (odd) parity of the value represented by a string.

	@param s bit string to check the parity of.
	@return true if parity matches, false else.
*)
let check_parity s =
	let l = String.length s in
	let calculated_parity_bit = calculate_parity (String.sub s 0 (pred l))
	and parity_bit = String.get s (pred l)
	in
	if calculated_parity_bit == parity_bit then
		true
	else
		false
	;;

(** Convert bit string to a character.

	@param s string with bits to convert.
	@param start_value ASCII value of character to start with.
	@return value of a character.
*)
let char_of_bits s start_value =
	let l = String.length s in
	let max2 = pow 2 l in
	let rec assemble_character i pow2 value =
		if pow2 == max2 then
			value
		else
			let next_value =
				match String.get s i with
				| '1' -> value + pow2
				|  _  -> value
			in
			assemble_character (succ i) (pow2 * 2) next_value
	in
	let result = assemble_character 0 1 start_value in
	Char.chr result
	;;

(** Convert character to a bit string.

	@param c character to convert.
	@param start_value ASCII value of character to start with.
	@param l length of the string to become.
	@return string with bit values.
*)
let bits_of_char c start_value l =
	let i = (Char.code c) - start_value
	and max2 = pow 2 (pred l) in
	let rec dissect_value value pow2 res =
		if pow2 == 0 then
			res
		else
			let next_value =
				if value >= pow2 then
					value - pow2
				else
					value
			and next_pow2 = pow2 / 2
			and ch =
				if value >= pow2 then
					"1"
				else
					"0"
			in
			dissect_value next_value next_pow2 (ch ^ res)
	in
	dissect_value i max2 ""
	;;

(** Calculate XOR operation between two characters.

	@param a first character ('1' or '0').
	@param b second character ('1' or '0').
	@raise Invalid_argument when either character is not 1 or 0.
	@return XOR result as a character.
*)
let char_xor a b =
	match (a, b) with
	| ('0', '0') -> '0'
	| ('0', '1') -> '1'
	| ('1', '0') -> '1'
	| ('1', '1') -> '0'
	|   _        -> raise (Invalid_argument "character is neither 0 nor 1")
	;;


(** Calculate LRC.

	@param lrc old LRC value.
	@param s bitstring to calculate the new LRC of.
	@raise Invalid_argument when either parameter is not 0 or 1.
	@return new LRC value.
*)
let calculate_lrc lrc s =
	let bits = String.length lrc in
	let res = String.create bits in
	let rec bit_loop i =
		if i == bits then
			res
		else
			let b = match String.get s i with
				| '1' -> '1'
				| '0' -> '0'
				|  _  -> raise (Invalid_argument
						"character is neither 0 nor 1")
			and a = String.get lrc i in
			let x = char_xor a b in
			String.set res i x;
			bit_loop (succ i)
	in
	bit_loop 0
	;;



(** Decode track.

	@param bitstring string which contains bitstream.
	@param bits width of a character in bits inclusive parity bit.
	@param vbits width of a character without parity bit.
	@param start_sentinel start sentinel of the track.
	@param end_sentinel end sentinel of the track.
	@param b2c bit string to character hash table.
	@param par bit string to parity hash table.
	@raise Failure when parity or checksum fail.
	@return decoded information.
*)
let decode_track bitstring bits vbits start_sentinel end_sentinel b2c par =
	(* Find start sentinel. *)
	let start_position = find_sentinel bitstring start_sentinel 0 1 in
	(* Find end sentinel. *)
	let end_position = find_sentinel bitstring end_sentinel
				(start_position + bits) bits in
	let characters = succ ((end_position - start_position) / bits) in
	(* Create result string. *)
	let result = String.create characters in
	let rec decoding_loop search_index result_index lrc =
		if search_index > end_position then
			(* Check checksum and it's parity *)
			let read_lrc = String.sub bitstring search_index bits
			and lrc_parity = Hashtbl.find par lrc in
			let complete_lrc = (lrc ^ Char.escaped lrc_parity) in
			if read_lrc = complete_lrc then
				()
			else
				raise (Failure "checksum failed")
		else
			let b = String.sub bitstring search_index bits in
			let value = String.sub b 0 vbits
			and parity = String.get b (pred bits) in
			if parity != (Hashtbl.find par value) then
				raise (Failure "parity failed")
			else
				let c = Hashtbl.find b2c value in
				String.set result result_index c;
				let new_lrc = calculate_lrc lrc value in
				decoding_loop
					(search_index + bits) (succ result_index) new_lrc
	in
	let initial_lrc = (String.make vbits '0')
	in
	decoding_loop start_position 0 initial_lrc;
	result;;

(** Encode track.

	@param s string which contains characters to encode.
	@param bits width of a character in bits inclusive parity bit.
	@param vbits width of a character without parity bit.
	@param c2b character to bit hash table.
	@param par bit string to parity hash table.
	@raise Not_found when a character is not in the character set.
	@return string with encoded information.
*)

let encode_track s bits vbits c2b p =
	(* Create buffer. *)
	let result = Buffer.create 1024
	and initial_lrc = String.make vbits '0'
	and length = String.length s in
	let rec processing_loop i lrc =
		if i == length then
		(
			(* Append LRC and it's parity, *)
			Buffer.add_string result lrc;
			Buffer.add_char result (Hashtbl.find p lrc)
		)
		else
			let c = String.get s i in
			let bitstring = Hashtbl.find c2b c in
			(* Add a character and it's parity. *)
			Buffer.add_string result bitstring;
			Buffer.add_char result (Hashtbl.find p bitstring);
			(* Calculate LRC. *)
			let new_lrc = calculate_lrc lrc bitstring in
			processing_loop (succ i) new_lrc
	in
	processing_loop 0 initial_lrc;
	Buffer.contents result
	;;


(** Read sound file. If more than one channel, take the first one.

	@param name name of the file to read.
	@raise Failure when not all data could be read.
	@return float array with data out of file.
*)
let read_sound_file name =
	let take_first a n =
		let length = Array.length a in
		let result = Array.make (length / n) 0.0 in
		let rec processing_loop i ir =
			if i >= length then
				result
			else
				let e = Array.get a i in
				Array.set result ir e;
				processing_loop (i + n) (succ ir)
		in
		processing_loop 0 0
	and file = Sndfile.openfile name in
	let channels = Sndfile.channels file
	and frames = Int64.to_int (Sndfile.frames file) in
	let data = Array.make frames 0.0 in
	let read_frames = Sndfile.read file data in
	Sndfile.close file;
	if frames != read_frames then
		raise (Failure "could not read all data");
	if channels > 1 then
		take_first data channels
	else
		data
	;;

(** Write sound file.

	@param name name of the file to write.
	@param content sound data to write.
	@raise Failure when not all data could be written.
*)
let write_sound_file name content =
	let frames = Array.length content
	and fmt = Sndfile.format Sndfile.MAJOR_WAV Sndfile.MINOR_PCM_16 in
	let file = Sndfile.openfile ~info:(Sndfile.WRITE, fmt, 1, 44100) name in
	let written_frames = Sndfile.write file content in
	if frames != written_frames then
		raise (Failure "could not write all data");
	Sndfile.close file
	;;


(** Decode aiken biphase.

	@param content contents from sound file.
	@param slnce_thr silence threshold, in positive float value.
	@param freq_thr frequency threshold, in percent.
	@raise Not_found when no data found.
	@raise Failure when processing wrong intervals.
	@return string with bits.
*)
let decode_aiken_biphase content slnce_thr freq_thr =
	let length = Array.length content
	and samples = Array.map (fun e -> abs_float e ) content in
	let rec traverse_silence i =
		if i == length then
			i
		else
			let sample = Array.get samples i in
			if sample > slnce_thr then
				i
			else
				traverse_silence (succ i)
	in
	let rec traverse_peak peak i =
		if i == length then
			(peak, i)
		else
			let sample = Array.get samples i in
			if sample <= slnce_thr then
				(peak, i)
			else
				let peak_sample = Array.get samples peak in
				let new_peak =
					if sample > peak_sample then
						i
					else
						peak
				in
				traverse_peak new_peak (succ i)
	in
	let rec calculate_peaks peaks peak ppeak i =
		if i == length then
			peaks
		else
			let previous_peak = peak
			and first_loud_sample = traverse_silence i
			and first_peak = 0 in
			let next_step =
				traverse_peak first_peak first_loud_sample in
			let current_peak = fst next_step
			and next_i = snd next_step in
			let interval = current_peak - previous_peak in
			let next_peaks =
				if interval > 0 then
					Array.append peaks [|interval|]
				else
					peaks
			in
			calculate_peaks
				next_peaks current_peak previous_peak next_i
	in
	let peaks = calculate_peaks [||] 0 0 0 in
	let peaks_length = Array.length peaks in
	if peaks_length < 2 then
		raise Not_found;
	let result = Buffer.create peaks_length in
	let deviation i thr = (thr * i) / 100 in
	let max_boundary i = i + deviation i freq_thr
	and min_boundary i = i - deviation i freq_thr
	in
	let rec calculate_bits zero_interval i =
		if i >= pred peaks_length then
			()
		else
			let one_interval = zero_interval / 2
			and pi = Array.get peaks i
			and pin = Array.get peaks (succ i) in
			let interval_length =
				if pi < max_boundary one_interval &&
				   pi > min_boundary one_interval &&
				   pin < max_boundary one_interval &&
				   pin > min_boundary one_interval then
				   1
				else if pi < max_boundary zero_interval &&
					pi > min_boundary zero_interval then
				   0
				else
				  -1
			in
			let new_zero_interval =
				match interval_length with
				|  1 -> pi * 2
				|  0 -> pi
				| -1 -> zero_interval
				| _ -> raise (Failure
					"character is neither 0 nor 1")
			and new_index =
				match interval_length with
				|  1 -> i + 2
				|  _ -> succ i
			and _ =
				match interval_length with
				|  1 -> Buffer.add_char result '1'
				|  0 -> Buffer.add_char result '0'
				|  _ -> ()
			in
			calculate_bits new_zero_interval new_index
	in
	calculate_bits (Array.get peaks 2) 0;
	Buffer.contents result
	;;

(** Encode aiken biphase.

	@param bitstring string with bits.
	@param swipe_speed expected swipe speed in mm/s, float.
	@param bit_density bits per track, int.
	@return float array with encoded information.
*)
let encode_aiken_biphase bitstring swipe_speed bit_density =
	let stripe_length = 85.73 (* standard length, in mm *)
	and frequency = 44100.0 (* also used in write_sound_file, in Hertz *) in
	let swipe_time = stripe_length /. swipe_speed in
	let swipe_time_bit = swipe_time /. (float_of_int bit_density) in
	let samples_zero = int_of_float (frequency *. swipe_time_bit) in
	let samples_one = samples_zero / 2 in
	let stripe_margin = String.make 8 '0' in
	(* Add margins to the bit sequence. *)
	let bits = stripe_margin ^ bitstring ^ stripe_margin in
	let length = String.length bits in
	let level_of d =
		if d then 1.0 else -1.0
	in
	let create_output l d =
		let v = level_of d in
		let output = Array.make l v in
		output
	in
	let create_zero d =
		create_output samples_zero d
	and create_one d =
		Array.append
			(create_output samples_one d)
			(create_output samples_one (not d))
	in
	let rec waveform_processing i w d =
		if i == length then
			w
		else
			let c = String.get bits i in
			let bit =
				match c with
				| '1' -> true
				| '0' -> false
				|  _  -> raise
					(Failure "character is not '0' or '1'")
			in
			let new_wave =
				if bit then
					create_one d
				else
					create_zero d
			in
			let next_wave = Array.append w new_wave
			and next_dir = if bit then d else (not d) in
			waveform_processing (succ i) next_wave next_dir
	(* Create waveform to start with. *)
	and start_waveform = create_zero false in
	(* Make bits to waveform. *)
	let waveform = waveform_processing 0 start_waveform true in
	waveform
	;;


(** Reverse string.

	@param s string to reverse.
	@return reversed string.
*)
let reverse_string s =
	let length = String.length s in
	let result = Buffer.create length in
	let rec processing_loop i =
		if i < 0 then
			Buffer.contents result
		else
			let c = String.get s i in
			Buffer.add_char result c;
			processing_loop (pred i)
	in
	processing_loop (pred length)
	;;

(** Initialize hashtables.

	@param b2c bits to character hash table.
	@param c2b character to bits hash table.
	@param p parity hash table.
	@param charset character set.
	@param charset_begin ASCII value of the first character in the charset.
	@param vbits width of the generated bit string.
*)
let init_hashtables b2c c2b p charset charset_begin vbits =
	Array.iter
		(fun c ->
			let bits = bits_of_char c charset_begin vbits in
			let parity = calculate_parity bits in
			Hashtbl.add b2c bits c;
			Hashtbl.add c2b c bits;
			Hashtbl.add p bits parity
		)
		charset
	;;


(* Program entry point. *)
let _ =
	init_hashtables iata_b2c iata_c2b iata_p
			iata_charset iata_charset_begin iata_vbits;
	init_hashtables aba_b2c aba_c2b aba_p
			aba_charset aba_charset_begin aba_vbits;
	init_hashtables thrift_b2c thrift_c2b thrift_p
			thrift_charset thrift_charset_begin thrift_vbits;
	print_endline "mcu - Magnetic Card Utility\n";

	let aba_track = "000000000000000000001101001101111000100010000000010000100010000010000100001010000001001101000010100000010000011100111100101100000101101100000000110000010001000000001001001110000001101010000111100000010000101000111111110000000000000" in
	let decoded_string =
		decode_track aba_track aba_bits aba_vbits
			aba_start_sentinel aba_end_sentinel aba_b2c aba_p in
	print_endline decoded_string;;


let silence_threshold = 5000.0 /. 32767.0;;
(* let frequency_threshold = 60.0 /. 100.0;; *)
let frequency_threshold = 60;; (* 60 percent *)


	let ar = read_sound_file "test12.wav";;
	let track = reverse_string (decode_aiken_biphase ar silence_threshold frequency_threshold);;
	let res = decode_track track aba_bits aba_vbits
			aba_start_sentinel aba_end_sentinel aba_b2c aba_p;;
	print_endline res;;

	let etrack = encode_track res aba_bits aba_vbits aba_c2b aba_p;;
	let ear = encode_aiken_biphase etrack 500.0 aba_bit_length;;
	let tf = "test666.wav";;
	if Sys.file_exists tf then
		Sys.remove tf;;
	let _ = write_sound_file "test666.wav" ear;;

	let dar = read_sound_file tf;;
	let dtrack = decode_aiken_biphase ar silence_threshold frequency_threshold;;
	let dres = decode_track track aba_bits aba_vbits
			aba_start_sentinel aba_end_sentinel aba_b2c aba_p;;
	print_endline dres;;


