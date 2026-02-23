(* stats.ml — Functional OCaml implementation of mean, median, mode
   Design:
   - Mean: float
   - Median: average of the two middle values (float) for even-length lists
   - Mode: return *all* values tying for highest frequency, sorted
   - Functional style: immutable data, List.* and Map, no mutation
*)

module IntMap = Map.Make (Int)

(* ---------- Core computations ---------- *)

let mean (xs : int list) : float =
  match xs with
  | [] -> invalid_arg "mean: empty list"
  | _ ->
      let sum, count =
        List.fold_left (fun (s, c) x -> (s + x, c + 1)) (0, 0) xs
      in
      float_of_int sum /. float_of_int count

let median (xs : int list) : float =
  match xs with
  | [] -> invalid_arg "median: empty list"
  | _ ->
      let s = List.sort compare xs in
      let n = List.length s in
      if n mod 2 = 1 then
        float_of_int (List.nth s (n / 2))
      else
        let a = List.nth s (n / 2 - 1)
        and b = List.nth s (n / 2) in
        (float_of_int (a + b)) /. 2.0

let freq_map (xs : int list) : int IntMap.t =
  List.fold_left
    (fun m x ->
      IntMap.update x (function None -> Some 1 | Some c -> Some (c + 1)) m)
    IntMap.empty xs

let modes (xs : int list) : int list =
  match xs with
  | [] -> invalid_arg "modes: empty list"
  | _ ->
      let m = freq_map xs in
      let maxf = IntMap.fold (fun _ c acc -> max c acc) m 0 in
      m
      |> IntMap.bindings               (* (value, count) list *)
      |> List.filter (fun (_v, c) -> c = maxf)
      |> List.map fst
      |> List.sort compare

(* ---------- Pretty print ---------- *)

let describe (xs : int list) : string =
  let m = mean xs in
  let med = median xs in
  let ms = modes xs in
  let modes_str =
    ms |> List.map string_of_int |> String.concat " "
  in
  Printf.sprintf
    "Count : %d\nMean  : %.6f\nMedian: %.6f\nMode  : [%s]\n"
    (List.length xs) m med modes_str

(* ---------- CLI ---------- *)

let parse_cli (argv : string array) : int list =
  if Array.length argv <= 1 then (
    prerr_endline ("Usage: " ^ argv.(0) ^ " <int1> <int2> ...");
    prerr_endline ("Example: " ^ argv.(0) ^ " 1 2 2 3 4 4 4 5");
    exit 1
  ) else
    let args = Array.sub argv 1 (Array.length argv - 1) |> Array.to_list in
    let to_int s =
      match int_of_string_opt s with
      | Some v -> v
      | None ->
          prerr_endline ("Invalid integer: " ^ s);
          exit 2
    in
    List.map to_int args

let () =
  let data = parse_cli Sys.argv in
  data |> describe |> print_string
