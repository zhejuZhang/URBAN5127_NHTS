function bidaxiao(int a, int b) {
  for (let i = 0; i < arguments.length; i++) {
    if (arguments[i] > a) {
      a = arguments[i];
    }
    if (arguments[i] < b) {
      b = arguments[i];
    }
  }
  return [a, b];
}

function maxmin(...args) {
  let max = args[0];
  let min = args[0];
  for (let i = 1; i < args.length; i++) {
    if (args[i] > max) {
      max = args[i];
    }
    if (args[i] < min) {
      min = args[i];
    }
  }
  return [max, min];
}