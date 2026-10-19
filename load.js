
import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  vus: 2000,
  duration: '600s',
};

export default function () {
  const res = http.get('https://minhajshahabah.sch.id/');

  check(res, {
    'status is 200': (r) => r.status === 200,
    'response under 1s': (r) => r.timings.duration < 1000,
  });

  sleep(1);
}
